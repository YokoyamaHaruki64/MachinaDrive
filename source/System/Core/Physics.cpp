#include "pch.h"

#include "System/Core/Physics.h"

#include "System/Core/PhysicsJoltUtils.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "Component/Rigidbody.h"
#include "Component/Transform.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// このファイルはゲーム側のComponentとJoltの境界である。
// Jolt固有のBody／Shape／callbackはBackendに閉じ込め、外部へはPhysicsとHitInfoだけを公開する。
namespace JoltPhysicsDetail
{
    // JoltのBody Layerは、広域判定を「動かないBody／動くBody」に絞るためだけに使う。
    // ゲーム側のCollider LayerはCompound Shape内で異なる可能性があるため、
    // 実際のLayer組み合わせはContactListener::OnContactValidateで判定する。
    constexpr uint32_t NON_MOVING_OBJECT_LAYER = 0;
    constexpr uint32_t MOVING_OBJECT_LAYER = 1;
    constexpr uint8_t NON_MOVING_BROAD_PHASE_LAYER = 0;
    constexpr uint8_t MOVING_BROAD_PHASE_LAYER = 1;

    // ゲーム側のUpdate間隔に依存しないよう、Joltは60Hzの固定刻みで進める。
    constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;
    constexpr float MAX_ACCUMULATED_TIME = FIXED_TIME_STEP * 4.0f;

    // 初期段階ではワールドを無制限に拡張せず、失敗時に原因を追いやすい上限にする。
    constexpr uint32_t MAX_BODIES = 8192;
    constexpr uint32_t MAX_BODY_PAIRS = 65536;
    constexpr uint32_t MAX_CONTACT_CONSTRAINTS = 65536;
    constexpr size_t TEMP_ALLOCATOR_SIZE = 32 * 1024 * 1024;
}

struct Physics::Backend
{
    using RawContactKey = PhysicsDetail::RawContactKey;
    using RawContactKeyHash = PhysicsDetail::RawContactKeyHash;
    using BodyRecord = PhysicsDetail::BodyRecord;
    using ContactState = PhysicsDetail::ContactState;
    using EventType = PhysicsDetail::EventType;
    using DispatchEvent = PhysicsDetail::DispatchEvent;

    // 内部型は専用ヘッダーに置き、Backend内では短い別名で扱う。

    // -------------------------------------------------------------------------
    // Joltの広域フィルターと接触callback
    // -------------------------------------------------------------------------

    class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
    {
    public:
        // BroadPhaseはShape同士の詳細判定より前に、静的Bodyと動的Bodyの組み合わせだけへ絞る。
        JPH::uint GetNumBroadPhaseLayers() const override
        {
            return 2;
        }

        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
        {
            return JPH::BroadPhaseLayer(
                layer == JoltPhysicsDetail::NON_MOVING_OBJECT_LAYER
                    ? JoltPhysicsDetail::NON_MOVING_BROAD_PHASE_LAYER
                    : JoltPhysicsDetail::MOVING_BROAD_PHASE_LAYER);
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
        {
            return layer.GetValue() == JoltPhysicsDetail::NON_MOVING_BROAD_PHASE_LAYER
                ? "NonMoving"
                : "Moving";
        }
#endif
    };

    class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        // Static同士は動かないため、広域判定の段階で除外する。
        bool ShouldCollide(
            JPH::ObjectLayer objectLayer,
            JPH::BroadPhaseLayer broadPhaseLayer) const override
        {
            if (objectLayer == JoltPhysicsDetail::NON_MOVING_OBJECT_LAYER)
            {
                return broadPhaseLayer.GetValue() == JoltPhysicsDetail::MOVING_BROAD_PHASE_LAYER;
            }
            return true;
        }
    };

    class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
    {
    public:
        // Jolt側にはBody単位の2種類のLayerだけを渡す。
        // Collider単位のLayerはここでは判断できないため、後段のValidateで確認する。
        bool ShouldCollide(JPH::ObjectLayer objectLayer1, JPH::ObjectLayer objectLayer2) const override
        {
            return objectLayer1 == JoltPhysicsDetail::MOVING_OBJECT_LAYER
                || objectLayer2 == JoltPhysicsDetail::MOVING_OBJECT_LAYER;
        }
    };

    class ContactListener final : public JPH::ContactListener
    {
        Backend& m_backend;

    public:
        explicit ContactListener(Backend& backend)
            : m_backend(backend)
        {
        }

        JPH::ValidateResult OnContactValidate(
            const JPH::Body& body1,
            const JPH::Body& body2,
            JPH::RVec3Arg,
            const JPH::CollideShapeResult& collisionResult) override
        {
            // このコールバックはJoltのワーカースレッドから呼ばれるため、
            // GameObjectやComponentの処理は行わず、読み取りだけに留める。
            BodyRecord* record1 = m_backend.FindBodyRecord(body1.GetID());
            BodyRecord* record2 = m_backend.FindBodyRecord(body2.GetID());
            if (!record1 || !record2)
            {
                return JPH::ValidateResult::RejectAllContactsForThisBodyPair;
            }

            Collider* collider1 = m_backend.FindCollider(*record1, collisionResult.mSubShapeID1);
            Collider* collider2 = m_backend.FindCollider(*record2, collisionResult.mSubShapeID2);

            if (!collider1 || !collider2)
            {
                // Compound Body内の別Colliderまで巻き込まないよう、接触単位で拒否する。
                return JPH::ValidateResult::RejectContact;
            }

            const auto toCollisionLayer = [](const Collider& collider)
            {
                return static_cast<Physics::CollisionLayer>(std::clamp(
                    collider.GetLayer(),
                    0,
                    static_cast<int>(Physics::MaxLayer - 1)));
            };
            const Physics::CollisionLayer layer1 = toCollisionLayer(*collider1);
            const Physics::CollisionLayer layer2 = toCollisionLayer(*collider2);

            if (!Physics::CanLayersCollide(layer1, layer2))
            {
                // Compound Body内の別Colliderまで巻き込まないよう、接触単位で拒否する。
                return JPH::ValidateResult::RejectContact;
            }

            // Compound Body内の別ColliderにもLayer判定を適用するため、
            // Bodyペア全体をAcceptせず、この接触だけを受理する。
            return JPH::ValidateResult::AcceptContact;
        }

        void OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& settings) override
        {
            // 接触情報はPhysics内部へコピーし、PhysicsSystem::Update完了後に通知する。
            m_backend.RecordContact(body1, body2, manifold, settings);
        }

        void OnContactPersisted(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& settings) override
        {
            // Persistedは前ステップから継続している接触。最新の法線・接触点へ更新する。
            m_backend.RecordContact(body1, body2, manifold, settings);
        }

        void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override
        {
            // 接触終了の確定はPhysicsSystem::Update後に接触キャッシュ全体を見て行う。
            // Sleep時にもRemovedが来るため、ここでは状態を消さない。
            (void)subShapePair;
        }
    };

    // -------------------------------------------------------------------------
    // Joltの実体とゲーム側への索引
    // -------------------------------------------------------------------------

    // Joltのワールド本体。ゲーム側のPhysicsWorldは作らず、このBackendだけが所有する。
    JPH::PhysicsSystem m_physicsSystem;

    // 固定ステップ中の一時メモリを毎回ヒープ確保しないための作業領域。
    JPH::TempAllocatorImpl m_tempAllocator{ JoltPhysicsDetail::TEMP_ALLOCATOR_SIZE };

    // Joltの並列衝突判定用。ゲーム側のComponentへ直接アクセスする処理はここでは行わない。
    JPH::JobSystemThreadPool m_jobSystem{
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        -1 };
    BroadPhaseLayerInterface m_broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilter m_objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilter m_objectLayerPairFilter;
    ContactListener m_contactListener;

    // BodyKeyは物理挙動で変化しないComponentアドレスだけを使う。
    // Position、Rotation、Velocityなどをキーに含めないため、動いても同じBodyを使い続けられる。
    std::unordered_map<void*, std::unique_ptr<BodyRecord>> m_bodyRecords;

    // Colliderから所属Bodyへ直接到達し、形状変更のたびに全Bodyを探索しない。
    std::unordered_map<Collider*, BodyRecord*> m_colliderToBody;

    // Componentの寿命だけを登録で追跡する。Bodyの追加・削除はこの集合へのイベントで行う。
    std::unordered_set<Collider*> m_registeredColliders;
    std::unordered_set<Rigidbody*> m_registeredRigidbodies;

    // Colliderが先にロードされた場合だけ、解決済みRigidbody単位で登録待ちにする。
    std::unordered_map<Rigidbody*, std::unordered_set<Collider*>> m_waitingColliders;
    std::unordered_map<Collider*, Rigidbody*> m_waitingRigidbodyByCollider;

    // 登録解除時に接触終了を次のPhysics::Updateへ渡すための一時イベント。
    std::vector<DispatchEvent> m_pendingEvents;

    // Jolt callbackで受け取ったBodyIDをゲーム側のBodyRecordへ戻すための索引。
    std::unordered_map<uint32_t, BodyRecord*> m_bodyRecordsByID;

    // ContactListenerはスレッドセーフにこのキャッシュへ記録し、後でBegin/Hit/Endへ確定する。
    std::unordered_map<RawContactKey, ContactState, RawContactKeyHash> m_contacts;
    std::mutex m_contactMutex;

    uint64_t m_currentStep{};
    float m_accumulatedTime{};
    bool m_isApplyingPhysicsTransforms{ false };

    static bool IsObjectActive(const GameObject* object)
    {
        for (const GameObject* current = object; current; current = current->GetParent())
        {
            if (current->IsDestroyed() || !current->IsActive())
            {
                return false;
            }
        }
        return true;
    }

    static bool IsInHierarchy(const GameObject* object, const GameObject* ancestor)
    {
        for (const GameObject* current = object; current; current = current->GetParent())
        {
            if (current == ancestor)
            {
                return true;
            }
        }
        return false;
    }

    Rigidbody* FindRegisteredRigidbody(Collider* collider) const
    {
        // Collider自身のGetRigidbodyが返す「最も近いRigidbody」を正とする。
        // 親Rigidbodyが無効でも、さらに上のRigidbodyへ勝手に所属させない。
        Rigidbody* rigidbody = collider->GetRigidbody();
        return rigidbody
            && m_registeredRigidbodies.contains(rigidbody)
            && IsObjectActive(rigidbody->GetOwner())
            ? rigidbody
            : nullptr;
    }

    BodyRecord* FindBodyRecord(void* key) const
    {
        const auto iterator = m_bodyRecords.find(key);
        return iterator != m_bodyRecords.end() ? iterator->second.get() : nullptr;
    }

    BodyRecord* FindBodyRecord(Collider* collider) const
    {
        const auto iterator = m_colliderToBody.find(collider);
        return iterator != m_colliderToBody.end() ? iterator->second : nullptr;
    }

    BodyRecord* CreateRecord(void* key, GameObject* bodyOwner, Rigidbody* rigidbody)
    {
        auto record = std::make_unique<BodyRecord>();
        record->bodyOwner = bodyOwner;
        record->rigidbody = rigidbody;
        BodyRecord* result = record.get();
        m_bodyRecords.emplace(key, std::move(record));
        return result;
    }

    void ClearWaitingCollider(Collider* collider)
    {
        const auto ownerIterator = m_waitingRigidbodyByCollider.find(collider);
        if (ownerIterator == m_waitingRigidbodyByCollider.end())
        {
            return;
        }

        Rigidbody* rigidbody = ownerIterator->second;
        const auto waitingIterator = m_waitingColliders.find(rigidbody);
        waitingIterator->second.erase(collider);
        if (waitingIterator->second.empty())
        {
            m_waitingColliders.erase(waitingIterator);
        }
        m_waitingRigidbodyByCollider.erase(ownerIterator);
    }

    void WaitForRigidbody(Collider* collider, Rigidbody* rigidbody)
    {
        ClearWaitingCollider(collider);
        m_waitingColliders[rigidbody].insert(collider);
        m_waitingRigidbodyByCollider[collider] = rigidbody;
    }

    BodyRecord* EnsureRigidbodyRecord(Rigidbody* rigidbody)
    {
        BodyRecord* record = FindBodyRecord(rigidbody);
        if (!record)
        {
            record = CreateRecord(rigidbody, rigidbody->GetOwner(), rigidbody);
        }

        return record;
    }

    void RemoveRecord(void* key, std::vector<DispatchEvent>& events)
    {
        const auto iterator = m_bodyRecords.find(key);
        if (iterator == m_bodyRecords.end())
        {
            return;
        }

        BodyRecord& record = *iterator->second;
        for (Collider* collider : record.colliders)
        {
            m_colliderToBody.erase(collider);
        }
        RemoveContactsForBody(record, events);
        DestroyBody(record);
        m_bodyRecords.erase(iterator);
    }

    void DetachCollider(Collider* collider, std::vector<DispatchEvent>& events)
    {
        BodyRecord* record = FindBodyRecord(collider);
        if (!record)
        {
            return;
        }

        if (!record->rigidbody)
        {
            RemoveRecord(collider, events);
            return;
        }

        m_colliderToBody.erase(collider);
        std::erase(record->colliders, collider);
        record->shapeDirty = true;
    }

    void AttachCollider(Collider* collider, std::vector<DispatchEvent>& events)
    {
        if (!m_registeredColliders.contains(collider))
        {
            return;
        }

        ClearWaitingCollider(collider);
        if (!IsObjectActive(collider->GetOwner()))
        {
            DetachCollider(collider, events);
            return;
        }

        Rigidbody* resolvedRigidbody = collider->GetRigidbody();
        Rigidbody* rigidbody = FindRegisteredRigidbody(collider);
        if (!rigidbody && resolvedRigidbody)
        {
            // 所属Rigidbodyが無効な間は、ColliderをStaticへ移さず待機させる。
            DetachCollider(collider, events);
            WaitForRigidbody(collider, resolvedRigidbody);
            return;
        }

        BodyRecord* currentRecord = FindBodyRecord(collider);
        BodyRecord* targetRecord = rigidbody
            ? EnsureRigidbodyRecord(rigidbody)
            : FindBodyRecord(collider);

        if (!rigidbody && !targetRecord)
        {
            targetRecord = CreateRecord(collider, collider->GetOwner(), nullptr);
        }

        if (currentRecord && currentRecord != targetRecord)
        {
            DetachCollider(collider, events);
        }

        if (std::find(targetRecord->colliders.begin(), targetRecord->colliders.end(), collider)
            == targetRecord->colliders.end())
        {
            targetRecord->colliders.push_back(collider);
            targetRecord->shapeDirty = true;
        }
        m_colliderToBody[collider] = targetRecord;

    }

    void RebindCollider(Collider* collider, std::vector<DispatchEvent>& events)
    {
        AttachCollider(collider, events);
        if (BodyRecord* record = FindBodyRecord(collider))
        {
            record->shapeDirty = true;
        }
    }

    void RebindCollider(Collider* collider)
    {
        RebindCollider(collider, m_pendingEvents);
    }

    void RegisterCollider(Collider* collider)
    {
        if (!collider || !collider->GetOwner() || !collider->GetOwner()->GetTransform())
        {
            return;
        }

        m_registeredColliders.insert(collider);
        AttachCollider(collider, m_pendingEvents);
    }

    void UnregisterCollider(Collider* collider)
    {
        m_registeredColliders.erase(collider);
        ClearWaitingCollider(collider);
        DetachCollider(collider, m_pendingEvents);
    }

    void RegisterRigidbody(Rigidbody* rigidbody)
    {
        if (!rigidbody || !rigidbody->GetOwner() || !rigidbody->GetOwner()->GetTransform())
        {
            return;
        }

        m_registeredRigidbodies.insert(rigidbody);
        if (IsObjectActive(rigidbody->GetOwner()))
        {
            EnsureRigidbodyRecord(rigidbody);
        }

        // Collider先行ロード時の待機集合だけを処理し、全Colliderは走査しない。
        const auto waitingIterator = m_waitingColliders.find(rigidbody);
        if (waitingIterator == m_waitingColliders.end())
        {
            return;
        }

        const std::vector<Collider*> waitingColliders(
            waitingIterator->second.begin(),
            waitingIterator->second.end());
        for (Collider* collider : waitingColliders)
        {
            AttachCollider(collider, m_pendingEvents);
        }
    }

    void UnregisterRigidbody(Rigidbody* rigidbody)
    {
        m_registeredRigidbodies.erase(rigidbody);

        BodyRecord* record = FindBodyRecord(rigidbody);
        std::vector<Collider*> colliders;
        if (record)
        {
            colliders = record->colliders;
            RemoveRecord(rigidbody, m_pendingEvents);
        }

        // 子Colliderは別の親Rigidbody、または独立Static Bodyへ明示的に移す。
        for (Collider* collider : colliders)
        {
            AttachCollider(collider, m_pendingEvents);
        }
    }

    void MarkBodyShapeDirty(Collider* collider)
    {
        if (BodyRecord* record = FindBodyRecord(collider))
        {
            record->shapeDirty = true;
        }
    }

    void MarkBodyShapeDirty(Rigidbody* rigidbody)
    {
        if (BodyRecord* record = FindBodyRecord(rigidbody))
        {
            record->shapeDirty = true;
        }
    }

    void MarkBodySettingsDirty(Rigidbody* rigidbody, bool requiresBodyRecreate)
    {
        if (BodyRecord* record = FindBodyRecord(rigidbody))
        {
            record->settingsDirty = true;
            record->settingsRequiresRecreate = record->settingsRequiresRecreate || requiresBodyRecreate;
        }
    }

    void MarkBodyVelocityDirty(Rigidbody* rigidbody)
    {
        if (BodyRecord* record = FindBodyRecord(rigidbody))
        {
            record->velocityDirty = true;
        }
    }

    void MarkBodyTransformDirty(Collider* collider)
    {
        if (m_isApplyingPhysicsTransforms)
        {
            return;
        }

        if (BodyRecord* record = FindBodyRecord(collider))
        {
            record->transformDirty = true;
        }
    }

    void MarkBodyTransformDirty(Rigidbody* rigidbody)
    {
        if (m_isApplyingPhysicsTransforms)
        {
            return;
        }

        if (BodyRecord* record = FindBodyRecord(rigidbody))
        {
            record->transformDirty = true;
        }
    }

    void RefreshRigidbodyState(Rigidbody* rigidbody)
    {
        if (!m_registeredRigidbodies.contains(rigidbody))
        {
            return;
        }

        if (IsObjectActive(rigidbody->GetOwner()))
        {
            EnsureRigidbodyRecord(rigidbody);
        }
        else
        {
            RemoveRecord(rigidbody, m_pendingEvents);
        }
    }

    void NotifyGameObjectStateChanged(GameObject* object)
    {
        // Active変更は低頻度なので、変更された階層に属する登録Componentだけを更新する。
        for (Collider* collider : m_registeredColliders)
        {
            if (IsInHierarchy(collider->GetOwner(), object))
            {
                RebindCollider(collider, m_pendingEvents);
            }
        }

        for (Rigidbody* rigidbody : m_registeredRigidbodies)
        {
            if (IsInHierarchy(rigidbody->GetOwner(), object))
            {
                RefreshRigidbodyState(rigidbody);
            }
        }
    }

    Backend()
        : m_contactListener(*this)
    {
        // Joltのフィルターと接触リスナーを登録する。BodyはComponent登録イベントで生成する。
        m_physicsSystem.Init(
            JoltPhysicsDetail::MAX_BODIES,
            0,
            JoltPhysicsDetail::MAX_BODY_PAIRS,
            JoltPhysicsDetail::MAX_CONTACT_CONSTRAINTS,
            m_broadPhaseLayerInterface,
            m_objectVsBroadPhaseLayerFilter,
            m_objectLayerPairFilter);
        m_physicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
        m_physicsSystem.SetContactListener(&m_contactListener);
    }

    ~Backend()
    {
        Clear();
    }

    // -------------------------------------------------------------------------
    // Body生成と接触情報のキャッシュ
    // -------------------------------------------------------------------------

    void CreateBody(BodyRecord& record)
    {
        // Bodyの生成時点でShapeと初期Transformを確定する。
        record.shape = PhysicsDetail::CreateShape(record);

        const Transform* transform = record.bodyOwner->GetTransform();
        const Vector3 position = transform->GetPosition();
        const Quaternion rotation = transform->GetRotation();
        const JPH::EMotionType motionType = PhysicsDetail::GetJoltMotionType(record.rigidbody);

        // JoltのBody Layerは広域判定用の2値へ変換する。細かなLayer判定はListenerで行う。
        const JPH::ObjectLayer objectLayer = motionType == JPH::EMotionType::Static
            ? JoltPhysicsDetail::NON_MOVING_OBJECT_LAYER
            : JoltPhysicsDetail::MOVING_OBJECT_LAYER;

        JPH::BodyCreationSettings settings(
            record.shape.GetPtr(),
            PhysicsDetail::ToJoltPosition(position),
            PhysicsDetail::ToJoltRotation(rotation),
            motionType,
            objectLayer);

        settings.mAllowDynamicOrKinematic = true;
        settings.mCollideKinematicVsNonDynamic = true;
        settings.mAllowedDOFs = PhysicsDetail::GetAllowedDOFs(record.rigidbody);

        if (record.rigidbody)
        {
            // Rigidbodyの初期値はBody生成時にJoltへ渡す。以後の変更はComponentのSetterから通知する。
            settings.mAllowSleeping = record.rigidbody->IsAllowSleep();
            settings.mFriction = record.rigidbody->GetFriction();
            settings.mRestitution = record.rigidbody->GetBounciness();
            settings.mGravityFactor = record.rigidbody->IsUseGravity() ? 1.0f : 0.0f;
            if (motionType == JPH::EMotionType::Dynamic)
            {
                settings.mLinearVelocity = JPH::Vec3(
                    record.rigidbody->GetVelocity().x,
                    record.rigidbody->GetVelocity().y,
                    record.rigidbody->GetVelocity().z);
                if (record.colliders.empty())
                {
                    // EmptyShapeは質量特性を自動計算できないため、単純な慣性を与える。
                    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
                    settings.mMassPropertiesOverride.mMass = std::max(record.rigidbody->GetMass(), 0.001f);
                    settings.mMassPropertiesOverride.mInertia = JPH::Mat44::sIdentity();
                }
                else
                {
                    // Shapeから慣性を計算しつつ、Inspectorで指定したmassを使う。
                    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                    settings.mMassPropertiesOverride.mMass = std::max(record.rigidbody->GetMass(), 0.001f);
                }
            }
        }

        JPH::BodyID bodyID = m_physicsSystem.GetBodyInterface().CreateAndAddBody(
            settings,
            JPH::EActivation::Activate);

        if (bodyID.IsInvalid())
        {
            Logger::Error("Jolt Bodyの生成に失敗しました。Body上限に達している可能性があります。");
            return;
        }

        record.bodyID = bodyID;
        record.shapeDirty = false;
        record.settingsDirty = false;
        record.settingsRequiresRecreate = false;
        record.velocityDirty = false;
        record.transformDirty = false;

        m_bodyRecordsByID[bodyID.GetIndexAndSequenceNumber()] = &record;
    }

    BodyRecord* FindBodyRecord(const JPH::BodyID& bodyID) const
    {
        // Callback中にBodyを直接保持せず、BodyIDの世代番号を含む値で安全に引き戻す。
        const auto iterator = m_bodyRecordsByID.find(bodyID.GetIndexAndSequenceNumber());
        return iterator != m_bodyRecordsByID.end() ? iterator->second : nullptr;
    }

    Collider* FindCollider(const BodyRecord& record, const JPH::SubShapeID& subShapeID) const
    {
        if (record.colliders.empty())
        {
            return nullptr;
        }

        // Collider番号はリーフShapeのUserDataへ保存しているため、Compound／Decoratorの
        // 実体へキャストせず、Joltの仮想APIでSubShapeを辿って逆引きする。
        const uint64_t userData = record.shape->GetSubShapeUserData(subShapeID);
        if (userData > 0 && userData <= record.colliders.size())
        {
            return record.colliders[static_cast<size_t>(userData - 1)];
        }

        // Colliderが1個だけの場合は、JoltがSubShapeIDを省略しても逆引きできる。
        return record.colliders.size() == 1 ? record.colliders.front() : nullptr;
    }

    void RecordContact(
        const JPH::Body& body1,
        const JPH::Body& body2,
        const JPH::ContactManifold& manifold,
        JPH::ContactSettings& settings)
    {
        // Joltの接触コールバックは並列実行されるため、ここではHitInfoを直接通知しない。
        // 最新の接触情報をキャッシュへ書き込み、メインスレッドのFinishContactsでイベント種別を決める。
        BodyRecord* record1 = FindBodyRecord(body1.GetID());
        BodyRecord* record2 = FindBodyRecord(body2.GetID());
        if (!record1 || !record2 || manifold.mRelativeContactPointsOn1.empty())
        {
            return;
        }

        Collider* collider1 = FindCollider(*record1, manifold.mSubShapeID1);
        Collider* collider2 = FindCollider(*record2, manifold.mSubShapeID2);
        if (!collider1 || !collider2)
        {
            return;
        }

        settings.mIsSensor = collider1->IsTrigger() || collider2->IsTrigger();

        // JoltのContactManifoldは複数点を持てるが、現段階のAPIでは代表点を1つだけ公開する。
        const JPH::RVec3 point = manifold.GetWorldSpaceContactPointOn1(0);
        const RawContactKey key{
            body1.GetID().GetIndexAndSequenceNumber(),
            manifold.mSubShapeID1.GetValue(),
            body2.GetID().GetIndexAndSequenceNumber(),
            manifold.mSubShapeID2.GetValue(),
        };

        ContactState state;
        state.body1 = record1;
        state.body2 = record2;
        state.collider1 = collider1;
        state.collider2 = collider2;
        state.point = PhysicsDetail::FromJoltPosition(point);
        // JoltのnormalはBody2をBody1から押し出す向きなので、
        // objectA側へ通知するnormalは反転する。
        state.normal = PhysicsDetail::FromJoltVector(-manifold.mWorldSpaceNormal);
        state.penetration = std::max(manifold.mPenetrationDepth, 0.0f);
        state.isTrigger = settings.mIsSensor;
        state.lastSeenStep = m_currentStep;

        std::scoped_lock lock(m_contactMutex);

        // 初めて見るキーはbegan=falseのままBeginになり、2回目以降はHitになる。
        ContactState& cachedState = m_contacts[key];
        state.began = cachedState.began;
        cachedState = state;
    }

    static HitInfo MakeHitInfo(const ContactState& state, bool reverse, bool hit)
    {
        // Joltが決めたBody順をそのまま外へ出さず、受信側を常にobjectAとして組み直す。
        HitInfo result;
        result.hit = hit;
        result.isTrigger = state.isTrigger;
        result.point = state.point;
        result.penetration = state.penetration;

        if (!reverse)
        {
            result.normal = state.normal;
            result.colliderA = state.collider1;
            result.colliderB = state.collider2;
        }
        else
        {
            result.normal = state.normal * -1.0f;
            result.colliderA = state.collider2;
            result.colliderB = state.collider1;
        }

        result.objectA = result.colliderA->GetOwner();
        result.objectB = result.colliderB->GetOwner();
        result.rigidbodyA = result.colliderA->GetRigidbody();
        result.rigidbodyB = result.colliderB->GetRigidbody();
        return result;
    }

    static void AppendEvents(
        std::vector<DispatchEvent>& events,
        const ContactState& state,
        EventType type)
    {
        // OnHit系APIは両側へ同じ接触を通知する。reverse側ではnormalとA/Bを反転する。
        events.push_back({ type, MakeHitInfo(state, false, type != EventType::End) });
        events.push_back({ type, MakeHitInfo(state, true, type != EventType::End) });
    }

    // -------------------------------------------------------------------------
    // 接触終了判定とBody構成の同期
    // -------------------------------------------------------------------------

    bool IsSleeping(const ContactState& state) const
    {
        // JoltはSleepしたBodyの接触コールバックを止めるため、
        // キャッシュが古くても相手がSleep中なら分離とは見なさない。
        auto isSleepingBody = [this](BodyRecord* record)
        {
            if (!record->rigidbody)
            {
                return false;
            }
            if (record->rigidbody->GetMotionType() != Rigidbody::MotionType::Dynamic)
            {
                return false;
            }
            return !m_physicsSystem.GetBodyInterfaceNoLock().IsActive(record->bodyID);
        };
        return isSleepingBody(state.body1) || isSleepingBody(state.body2);
    }

    void RemoveContactsForBody(BodyRecord& record, std::vector<DispatchEvent>& events)
    {
        // Bodyを消す前にキャッシュから接触を取り除く。
        // すでに相手が無効なら通知せず、生存している相手側だけへEndを送る。
        std::scoped_lock lock(m_contactMutex);
        for (auto iterator = m_contacts.begin(); iterator != m_contacts.end();)
        {
            if (iterator->second.body1 == &record || iterator->second.body2 == &record)
            {
                ContactState state = iterator->second;
                BodyRecord* other = state.body1 == &record ? state.body2 : state.body1;
                if (state.began
                    && other->bodyOwner->IsActive() && !other->bodyOwner->IsDestroyed())
                {
                    const bool recordIsFirst = state.body1 == &record;
                    events.push_back({
                        EventType::End,
                        MakeHitInfo(state, recordIsFirst, false) });
                }
                iterator = m_contacts.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }
    }

    void DestroyBody(BodyRecord& record)
    {
        // JoltのBodyIDを索引から外してからBodyをRemove/Destroyする。
        // 先に索引を消すことで、破棄中のcallbackが古いBodyRecordを引かないようにする。
        if (record.bodyID.IsInvalid())
        {
            return;
        }
        m_bodyRecordsByID.erase(record.bodyID.GetIndexAndSequenceNumber());
        m_physicsSystem.GetBodyInterface().RemoveBody(record.bodyID);
        m_physicsSystem.GetBodyInterface().DestroyBody(record.bodyID);
        record.bodyID = JPH::BodyID();
    }

    void RecreateBody(BodyRecord& record, std::vector<DispatchEvent>& events)
    {
        // CreateBodyが現在のTransform・速度・設定をまとめて反映する。
        RemoveContactsForBody(record, events);
        DestroyBody(record);
        CreateBody(record);
    }

    // -------------------------------------------------------------------------
    // 物理ステップ前後のTransform／速度同期
    // -------------------------------------------------------------------------

    void SyncBody(BodyRecord& record, float deltaTime)
    {
        auto& bodyInterface = m_physicsSystem.GetBodyInterface();
        const JPH::EMotionType motionType = PhysicsDetail::GetJoltMotionType(record.rigidbody);

        if (record.transformDirty)
        {
            const Transform* transform = record.bodyOwner->GetTransform();
            const Vector3 position = transform->GetPosition();
            const Quaternion rotation = transform->GetRotation();
            if (motionType == JPH::EMotionType::Kinematic)
            {
                bodyInterface.MoveKinematic(
                    record.bodyID,
            PhysicsDetail::ToJoltPosition(position),
            PhysicsDetail::ToJoltRotation(rotation),
                    deltaTime);
            }
            else
            {
                const JPH::EActivation activation = motionType == JPH::EMotionType::Dynamic
                    ? JPH::EActivation::Activate
                    : JPH::EActivation::DontActivate;
                bodyInterface.SetPositionAndRotation(
                    record.bodyID,
            PhysicsDetail::ToJoltPosition(position),
            PhysicsDetail::ToJoltRotation(rotation),
                    activation);
            }
            record.transformDirty = false;
        }

        if (record.rigidbody && motionType == JPH::EMotionType::Dynamic && record.velocityDirty)
        {
            // 速度変更はRigidbodyのSetterから通知されるため、毎Frameの値比較は行わない。
            const Vector3 requestedVelocity = record.rigidbody->GetVelocity();
            bodyInterface.SetLinearVelocity(
                record.bodyID,
                JPH::Vec3(requestedVelocity.x, requestedVelocity.y, requestedVelocity.z));
            record.velocityDirty = false;
        }
    }

    void SyncBodies(float deltaTime, std::vector<DispatchEvent>& events)
    {
        // Shape／Settingsの変更はComponentから通知されたBodyだけがdirtyになる。
        // ここではdirty flagを処理するが、ColliderやRigidbodyの現在値を比較しない。
        for (auto& [key, recordPointer] : m_bodyRecords)
        {
            (void)key;
            BodyRecord& record = *recordPointer;
            if (record.bodyID.IsInvalid())
            {
                CreateBody(record);
            }

            if (record.bodyID.IsInvalid())
            {
                continue;
            }

            if (record.shapeDirty || (record.settingsDirty && record.settingsRequiresRecreate))
            {
                // 現段階では低頻度の形状変更をBody再生成で反映する。
                // 高頻度更新が必要になった時点でMutableCompoundShapeを検討する。
                RecreateBody(record, events);
            }
            else if (record.settingsDirty && record.rigidbody)
            {
                auto& bodyInterface = m_physicsSystem.GetBodyInterface();
                bodyInterface.SetFriction(record.bodyID, record.rigidbody->GetFriction());
                bodyInterface.SetRestitution(record.bodyID, record.rigidbody->GetBounciness());
        if (PhysicsDetail::GetJoltMotionType(record.rigidbody) != JPH::EMotionType::Static)
                {
                    bodyInterface.SetGravityFactor(
                        record.bodyID,
                        record.rigidbody->IsUseGravity() ? 1.0f : 0.0f);
                }
                record.settingsDirty = false;
                record.settingsRequiresRecreate = false;
            }

            SyncBody(record, deltaTime);
        }
    }

    void SyncDynamicBodies()
    {
        // JoltのDynamic結果をゲーム側へ戻す。Transformの更新はこの1箇所に限定する。
        const auto& bodyInterface = m_physicsSystem.GetBodyInterfaceNoLock();
        m_isApplyingPhysicsTransforms = true;
        for (auto& [key, recordPointer] : m_bodyRecords)
        {
            (void)key;
            BodyRecord& record = *recordPointer;
            if (!record.rigidbody || PhysicsDetail::GetJoltMotionType(record.rigidbody) != JPH::EMotionType::Dynamic
                || record.bodyID.IsInvalid())
            {
                continue;
            }

            // BodyのWorld値をTransformへ戻し、親を持つ場合は既存のWorld setterにLocal変換させる。
            JPH::RVec3 position;
            JPH::Quat rotation;
            bodyInterface.GetPositionAndRotation(record.bodyID, position, rotation);
            const Vector3 worldPosition = PhysicsDetail::FromJoltPosition(position);
            const Quaternion worldRotation = PhysicsDetail::FromJoltRotation(rotation);
            Transform* transform = record.bodyOwner->GetTransform();
            transform->SetPosition(worldPosition);
            transform->SetRotation(worldRotation);

            const Vector3 velocity = PhysicsDetail::FromJoltVector(bodyInterface.GetLinearVelocity(record.bodyID));
            record.rigidbody->SetVelocityFromPhysics(velocity);
        }
        m_isApplyingPhysicsTransforms = false;
    }

    std::vector<DispatchEvent> FinishContacts()
    {
        // 今回の固定ステップで見つかった接触と前回キャッシュを比較し、
        // Begin／Hit／Endのいずれかへ確定する。
        std::vector<DispatchEvent> events;
        std::scoped_lock lock(m_contactMutex);

        for (auto iterator = m_contacts.begin(); iterator != m_contacts.end();)
        {
            ContactState& state = iterator->second;
            if (state.lastSeenStep == m_currentStep)
            {
                // 今ステップでも見つかった接触は、初回だけBegin、それ以降はHitにする。
                AppendEvents(events, state, state.began ? EventType::Hit : EventType::Begin);
                state.began = true;
                ++iterator;
                continue;
            }

            if (IsSleeping(state))
            {
                // SleepによるJoltのRemovedは本当の分離ではないため、接触を保持する。
                AppendEvents(events, state, EventType::Hit);
                ++iterator;
                continue;
            }

            AppendEvents(events, state, EventType::End);
            iterator = m_contacts.erase(iterator);
        }
        return events;
    }

    std::vector<DispatchEvent> Step(float deltaTime)
    {
        // 1フレーム分の入力を受け取り、明示通知の反映→Body同期→固定刻み→結果同期の順で処理する。
        std::vector<DispatchEvent> events = std::move(m_pendingEvents);
        m_pendingEvents.clear();
        SyncBodies(deltaTime, events);

        m_accumulatedTime = std::min(
            m_accumulatedTime + std::max(deltaTime, 0.0f),
            JoltPhysicsDetail::MAX_ACCUMULATED_TIME);
        int stepCount = 0;
        while (m_accumulatedTime >= JoltPhysicsDetail::FIXED_TIME_STEP && stepCount < 4)
        {
            // フレーム落ちでdeltaTimeが大きくなっても、1フレーム最大4ステップに制限する。
            m_accumulatedTime -= JoltPhysicsDetail::FIXED_TIME_STEP;
            ++m_currentStep;
            const JPH::EPhysicsUpdateError error = m_physicsSystem.Update(
                JoltPhysicsDetail::FIXED_TIME_STEP,
                1,
                &m_tempAllocator,
                &m_jobSystem);
            if (error != JPH::EPhysicsUpdateError::None)
            {
                Logger::Error("Jolt PhysicsSystem::Updateに失敗しました。");
                break;
            }

            SyncDynamicBodies();

            // ContactListenerが並列に蓄積した情報を、ここでメインスレッド用イベントへ変換する。
            std::vector<DispatchEvent> contactEvents = FinishContacts();
            events.insert(events.end(), contactEvents.begin(), contactEvents.end());
            ++stepCount;
        }
        return events;
    }

    void Clear()
    {
        // Play終了やPhysics終了時は、GameObjectが先に破棄されている可能性があるため、
        // 接触Endを通知せずキャッシュとJolt Bodyを静かに破棄する。
        m_contacts.clear();
        m_bodyRecordsByID.clear();
        for (auto& [key, record] : m_bodyRecords)
        {
            (void)key;
            DestroyBody(*record);
        }
        m_bodyRecords.clear();
        m_colliderToBody.clear();
        m_registeredColliders.clear();
        m_registeredRigidbodies.clear();
        m_waitingColliders.clear();
        m_waitingRigidbodyByCollider.clear();
        m_pendingEvents.clear();
        m_accumulatedTime = 0.0f;
    }
};

// -----------------------------------------------------------------------------
// Physicsの公開API。ここからGameObjectへ接触イベントをメインスレッドで通知する。
// -----------------------------------------------------------------------------

std::unique_ptr<Physics::Backend> Physics::s_backend;

bool Physics::Initialize()
{
    if (s_backend)
    {
        return true;
    }

    // Joltの型登録はPhysics Backendより先に行い、Backend破棄後にUnregisterTypesする。
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    s_backend = std::make_unique<Backend>();
    return true;
}

void Physics::Uninitialize()
{
    if (!s_backend)
    {
        return;
    }

    // BackendがBodyとContactListenerを解放した後で、Joltの型登録を解除する。
    s_backend.reset();
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void Physics::Update(float deltaTime)
{
    if (!s_backend)
    {
        return;
    }

    // Backend内ではJolt固有処理だけを行い、ここで初めてGameObjectへ通知する。
    std::vector<Backend::DispatchEvent> events = s_backend->Step(deltaTime);

    // unordered_map由来のイベント順に依存しないよう、通知順をObject IDで固定する。
    std::sort(events.begin(), events.end(), [](const Backend::DispatchEvent& a, const Backend::DispatchEvent& b)
    {
        const uint64_t aObject = a.hitInfo.objectA->GetId();
        const uint64_t bObject = b.hitInfo.objectA->GetId();
        if (aObject != bObject)
        {
            return aObject < bObject;
        }
        const uint64_t aOther = a.hitInfo.objectB->GetId();
        const uint64_t bOther = b.hitInfo.objectB->GetId();
        return aOther < bOther;
    });

    for (const Backend::DispatchEvent& event : events)
    {
        // Destroy済みObjectへのcallbackは送らない。相手側のイベントは別のDispatchEventで処理される。
        GameObject* object = event.hitInfo.objectA;
        if (object->IsDestroyed() || !object->IsActive())
        {
            continue;
        }

        switch (event.type)
        {
        case Backend::EventType::Begin:
            object->NotifyHitBegin(event.hitInfo);
            break;
        case Backend::EventType::Hit:
            object->NotifyHit(event.hitInfo);
            break;
        case Backend::EventType::End:
            object->NotifyHitEnd(event.hitInfo);
            break;
        }
    }
}

void Physics::RegisterCollider(Collider* collider)
{
    if (s_backend)
    {
        s_backend->RegisterCollider(collider);
    }
}

void Physics::UnregisterCollider(Collider* collider)
{
    if (s_backend)
    {
        s_backend->UnregisterCollider(collider);
    }
}

void Physics::RegisterRigidbody(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->RegisterRigidbody(rigidbody);
    }
}

void Physics::UnregisterRigidbody(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->UnregisterRigidbody(rigidbody);
    }
}

void Physics::MarkBodyShapeDirty(Collider* collider)
{
    if (s_backend)
    {
        s_backend->MarkBodyShapeDirty(collider);
    }
}

void Physics::MarkBodyShapeDirty(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->MarkBodyShapeDirty(rigidbody);
    }
}

void Physics::MarkBodySettingsDirty(Rigidbody* rigidbody, bool requiresBodyRecreate)
{
    if (s_backend)
    {
        s_backend->MarkBodySettingsDirty(rigidbody, requiresBodyRecreate);
    }
}

void Physics::MarkBodyVelocityDirty(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->MarkBodyVelocityDirty(rigidbody);
    }
}

void Physics::MarkBodyTransformDirty(Collider* collider)
{
    if (s_backend)
    {
        s_backend->MarkBodyTransformDirty(collider);
    }
}

void Physics::MarkBodyTransformDirty(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->MarkBodyTransformDirty(rigidbody);
    }
}

void Physics::RebindCollider(Collider* collider)
{
    if (s_backend)
    {
        s_backend->RebindCollider(collider);
    }
}

void Physics::RefreshRigidbodyState(Rigidbody* rigidbody)
{
    if (s_backend)
    {
        s_backend->RefreshRigidbodyState(rigidbody);
    }
}

void Physics::NotifyGameObjectStateChanged(GameObject* object)
{
    if (s_backend)
    {
        s_backend->NotifyGameObjectStateChanged(object);
    }
}

void Physics::Clear()
{
    if (!s_backend)
    {
        return;
    }

    // BodyRecordはGameObject／Colliderのアドレスを保持しているため、
    // GameObjectの破棄より先にBackend側の参照を全て外す。
    s_backend->Clear();
}

bool Physics::IsInitialized()
{
    return s_backend != nullptr;
}

bool Physics::CanLayersCollide(CollisionLayer layerA, CollisionLayer layerB)
{
    // Layer番号の意味はPhysics.hのCollisionPairsに集約し、Collider側にMaskを持たせない。
    if (layerA >= MaxLayer || layerB >= MaxLayer)
    {
        return false;
    }

    for (const auto& pair : CollisionPairs)
    {
        if ((pair.first == layerA && pair.second == layerB)
            || (pair.first == layerB && pair.second == layerA))
        {
            return true;
        }
    }
    return false;
}
