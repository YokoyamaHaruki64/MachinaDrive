#include "pch.h"

#include "System/Core/Physics.h"

#define JPH_USE_AVX2
#define JPH_USE_AVX
#define JPH_USE_SSE4_1
#define JPH_USE_SSE4_2
#define JPH_USE_LZCNT
#define JPH_USE_TZCNT
#define JPH_USE_F16C
#define JPH_USE_FMADD

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "Component/BoxCollider.h"
#include "Component/CapsuleCollider.h"
#include "Component/Rigidbody.h"
#include "Component/SphereCollider.h"
#include "Component/Transform.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"

#include <algorithm>
#include <bit>
#include <cmath>
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
    // -------------------------------------------------------------------------
    // 接触とBodyをゲーム側へ戻すためのデータ構造
    // -------------------------------------------------------------------------

    // 接触したBodyだけでなくCompound ShapeのSubShapeもキーに含める。
    // これにより、親Bodyに複数Colliderがある場合もCollider単位でBegin/Endを管理できる。
    struct RawContactKey
    {
        uint32_t body1{};
        uint32_t subShape1{};
        uint32_t body2{};
        uint32_t subShape2{};

        bool operator==(const RawContactKey& other) const
        {
            return body1 == other.body1
                && subShape1 == other.subShape1
                && body2 == other.body2
                && subShape2 == other.subShape2;
        }
    };

    struct RawContactKeyHash
    {
        size_t operator()(const RawContactKey& key) const
        {
            size_t result = key.body1;
            result = result * 31 + key.subShape1;
            result = result * 31 + key.body2;
            result = result * 31 + key.subShape2;
            return result;
        }
    };

    struct BodyRecord;

    struct ContactState
    {
        // Joltのワーカースレッドから受け取った接触情報を、メインスレッドのイベントへ変換する中間状態。
        BodyRecord* body1{};
        BodyRecord* body2{};
        Collider* collider1{};
        Collider* collider2{};
        Vector3 point{ Vector3::Zero() };
        Vector3 normal{ Vector3::Up() };
        float penetration{};
        bool isTrigger{};
        uint64_t lastSeenStep{};
        bool began{};
    };

    enum class EventType
    {
        Begin,
        Hit,
        End,
    };

    struct DispatchEvent
    {
        EventType type{};
        HitInfo hitInfo{};
    };

    struct BodyRecord
    {
        // keyはRigidbodyまたはColliderのアドレス。Rigidbodyがある場合は子Colliderも同じBodyへまとめる。
        void* key{};
        GameObject* bodyOwner{};
        Rigidbody* rigidbody{};
        std::vector<Collider*> colliders;
        JPH::BodyID bodyID;
        JPH::ShapeRefC shape;
        // どちらもBodyの同一性ではなく、Jolt Bodyを差し替える必要があるかの判定値。
        // 同一性はkeyのRigidbody／Colliderポインタだけで決まり、物理挙動の値は含めない。
        size_t shapeSignature{};
        size_t settingsSignature{};
        bool hasPhysicsTransform{ false };
        Vector3 lastPhysicsPosition{ Vector3::Zero() };
        Quaternion lastPhysicsRotation{ Quaternion::Identity() };
        Vector3 lastPhysicsVelocity{ Vector3::Zero() };
        bool hasPhysicsVelocity{ false };
    };

    struct DesiredBody
    {
        // 構成変更が検出されたFrameに、登録集合から組み立てる「あるべきBody」の一時データ。
        // 既存Bodyとの差分だけをRefreshBodiesで反映する。
        void* key{};
        GameObject* bodyOwner{};
        Rigidbody* rigidbody{};
        std::vector<Collider*> colliders;
    };

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

    // ManagerのGameObject配列を毎Step走査する代わりに、物理対象の寿命だけを登録で追跡する。
    std::unordered_set<Collider*> m_registeredColliders;
    std::unordered_set<Rigidbody*> m_registeredRigidbodies;

    // 構成変更があったFrameだけ、登録集合からBody集合を再構築する。
    bool m_structureDirty{ true };

    // Jolt callbackで受け取ったBodyIDをゲーム側のBodyRecordへ戻すための索引。
    std::unordered_map<uint32_t, BodyRecord*> m_bodyRecordsByID;

    // ContactListenerはスレッドセーフにこのキャッシュへ記録し、後でBegin/Hit/Endへ確定する。
    std::unordered_map<RawContactKey, ContactState, RawContactKeyHash> m_contacts;
    std::mutex m_contactMutex;

    uint64_t m_currentStep{};
    float m_accumulatedTime{};

    void RegisterCollider(Collider* collider)
    {
        if (collider && m_registeredColliders.insert(collider).second)
        {
            m_structureDirty = true;
        }
    }

    void UnregisterCollider(Collider* collider)
    {
        if (collider && m_registeredColliders.erase(collider) > 0)
        {
            m_structureDirty = true;
        }
    }

    void RegisterRigidbody(Rigidbody* rigidbody)
    {
        if (rigidbody && m_registeredRigidbodies.insert(rigidbody).second)
        {
            m_structureDirty = true;
        }
    }

    void UnregisterRigidbody(Rigidbody* rigidbody)
    {
        if (rigidbody && m_registeredRigidbodies.erase(rigidbody) > 0)
        {
            m_structureDirty = true;
        }
    }

    void MarkDirty()
    {
        m_structureDirty = true;
    }

    Backend()
        : m_contactListener(*this)
    {
        // Joltのフィルターと接触リスナーを登録する。BodyはRefreshBodiesで遅延生成する。
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
    // Joltとゲーム側の値変換、Shape生成
    // -------------------------------------------------------------------------

    static JPH::RVec3 ToJoltPosition(const Vector3& value)
    {
        // 現在のプロジェクト設定では軸変換を行わず、位置成分をそのまま境界越しに渡す。
        return JPH::RVec3(value.x, value.y, value.z);
    }

    static JPH::Quat ToJoltRotation(const Quaternion& value)
    {
        // 不正な長さのQuaternionがBodyへ入らないよう、境界で正規化する。
        const Quaternion normalized = value.Normalized();
        return JPH::Quat(normalized.x, normalized.y, normalized.z, normalized.w);
    }

    static Vector3 FromJoltVector(JPH::Vec3 value)
    {
        return Vector3(value.GetX(), value.GetY(), value.GetZ());
    }

    static Quaternion FromJoltRotation(JPH::Quat value)
    {
        return Quaternion(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
    }

    static Vector3 FromJoltPosition(JPH::RVec3 value)
    {
        return Vector3(
            static_cast<float>(value.GetX()),
            static_cast<float>(value.GetY()),
            static_cast<float>(value.GetZ()));
    }

    static bool NearlyEqual(const Vector3& a, const Vector3& b)
    {
        return Vector3::DistanceSqr(a, b) <= 1.0e-8f;
    }

    static bool NearlyEqual(const Quaternion& a, const Quaternion& b)
    {
        return std::abs(a.x - b.x) <= 1.0e-4f
            && std::abs(a.y - b.y) <= 1.0e-4f
            && std::abs(a.z - b.z) <= 1.0e-4f
            && std::abs(a.w - b.w) <= 1.0e-4f;
    }

    static void HashCombine(size_t& hash, size_t value)
    {
        hash ^= value + static_cast<size_t>(0x9e3779b9) + (hash << 6) + (hash >> 2);
    }

    static void HashFloat(size_t& hash, float value)
    {
        HashCombine(hash, std::bit_cast<uint32_t>(value));
    }

    static void HashVector(size_t& hash, const Vector3& value)
    {
        HashFloat(hash, value.x);
        HashFloat(hash, value.y);
        HashFloat(hash, value.z);
    }

    static void HashQuaternion(size_t& hash, const Quaternion& value)
    {
        HashFloat(hash, value.x);
        HashFloat(hash, value.y);
        HashFloat(hash, value.z);
        HashFloat(hash, value.w);
    }

    static Vector3 GetWorldScale(const Transform* transform)
    {
        return transform ? transform->GetScale() : Vector3::One();
    }

    static JPH::ShapeRefC CreateLeafShape(Collider& collider)
    {
        // JoltのShapeはBodyから見た単位形状として作り、位置・回転はCompound側で与える。
        // 非一様ScaleをJoltのShapeへ直接渡すのではなく、形状寸法へ焼き込む。
        JPH::ShapeRefC shape;
        const Transform* transform = collider.transform();
        const Vector3 scale = GetWorldScale(transform);

        if (const BoxCollider* box = dynamic_cast<const BoxCollider*>(&collider))
        {
            // Boxは各軸のScaleを半サイズへ適用する。
            const Vector3 size = box->GetSize();

            const Vector3 halfExtent(
                std::max(std::abs(size.x * scale.x) * 0.5f, 0.001f),
                std::max(std::abs(size.y * scale.y) * 0.5f, 0.001f),
                std::max(std::abs(size.z * scale.z) * 0.5f, 0.001f));
            shape = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));
        }
        else if (const SphereCollider* sphere = dynamic_cast<const SphereCollider*>(&collider))
        {
            // Sphereは球形を保つため、最も大きい軸のScaleを半径へ使う。
            const float radius = std::max(
                std::abs(sphere->GetRadius())
                    * std::max({ std::abs(scale.x), std::abs(scale.y), std::abs(scale.z) }),
                0.001f);
            shape = new JPH::SphereShape(radius);
        }
        else if (const CapsuleCollider* capsule = dynamic_cast<const CapsuleCollider*>(&collider))
        {
            // Capsuleの中心線はY軸。半径はXZ、全高はYのScaleで決める。
            const float radius = std::max(
                std::abs(capsule->GetRadius())
                    * std::max(std::abs(scale.x), std::abs(scale.z)),
                0.001f);
            const float halfHeight = std::abs(capsule->GetHeight() * scale.y) * 0.5f;

            const float halfHeightOfCylinder = std::max(halfHeight - radius, 0.001f);
            shape = new JPH::CapsuleShape(halfHeightOfCylinder, radius);
        }

        return shape;
    }

    static size_t CalculateShapeSignature(
        const std::vector<Collider*>& colliders,
        const Transform* bodyTransform)
    {
        // Body自身のWorld移動・回転はShapeを変えないため、Body基準の相対値だけを署名に含める。
        // World値をそのまま含めると、Dynamic Bodyが1フレーム動くたびにShapeを再生成してしまう。
        size_t hash = 0;
        const Vector3 bodyPosition = bodyTransform ? bodyTransform->GetPosition() : Vector3::Zero();
        const Quaternion inverseBodyRotation = bodyTransform
            ? bodyTransform->GetRotation().Inversed()
            : Quaternion::Identity();

        for (Collider* collider : colliders)
        {
            HashCombine(hash, reinterpret_cast<size_t>(collider));
            HashCombine(hash, static_cast<size_t>(collider->GetShapeType()));
            const Transform* colliderTransform = collider->transform();

            const Vector3 relativePosition = inverseBodyRotation.RotateVector(
                collider->GetWorldCenter() - bodyPosition);
            const Quaternion relativeRotation = inverseBodyRotation
                * colliderTransform->GetRotation();

            HashVector(hash, relativePosition);
            HashQuaternion(hash, relativeRotation);
            HashVector(hash, GetWorldScale(colliderTransform));
            HashCombine(hash, static_cast<size_t>(collider->GetLayer()));
            HashCombine(hash, collider->IsTrigger() ? 1 : 0);

            if (const BoxCollider* box = dynamic_cast<const BoxCollider*>(collider))
            {
                HashVector(hash, box->GetSize());
            }
            else if (const SphereCollider* sphere = dynamic_cast<const SphereCollider*>(collider))
            {
                HashFloat(hash, sphere->GetRadius());
            }
            else if (const CapsuleCollider* capsule = dynamic_cast<const CapsuleCollider*>(collider))
            {
                HashFloat(hash, capsule->GetRadius());
                HashFloat(hash, capsule->GetHeight());
            }
        }
        return hash;
    }

    static size_t CalculateSettingsSignature(Rigidbody* rigidbody)
    {
        if (!rigidbody)
        {
            return 0;
        }

        size_t hash = 0;
        HashCombine(hash, static_cast<size_t>(rigidbody->GetMotionType()));
        HashFloat(hash, rigidbody->GetMass());
        HashFloat(hash, rigidbody->GetBounciness());
        HashFloat(hash, rigidbody->GetFriction());
        HashCombine(hash, rigidbody->IsUseGravity() ? 1 : 0);
        HashCombine(hash, rigidbody->IsAllowSleep() ? 1 : 0);
        HashCombine(hash, rigidbody->IsFreezeRotation() ? 1 : 0);
        return hash;
    }

    JPH::ShapeRefC CreateShape(BodyRecord& record)
    {
        // ColliderなしRigidbodyも、速度・重力・Sleepを持つBodyとして登録する。
        // EmptyShapeは他のShapeと接触しないため、衝突対象だけを持たせたい場合の安全な空形状になる。
        if (record.colliders.empty())
        {
            return JPH::ShapeRefC(new JPH::EmptyShape());
        }

        const Transform* bodyTransform = record.bodyOwner ? record.bodyOwner->GetTransform() : nullptr;
        if (!bodyTransform)
        {
            return JPH::ShapeRefC(new JPH::EmptyShape());
        }

        const Vector3 bodyPosition = bodyTransform->GetPosition();
        const Quaternion inverseBodyRotation = bodyTransform->GetRotation().Inversed();
        JPH::StaticCompoundShapeSettings settings;

        for (size_t index = 0; index < record.colliders.size(); ++index)
        {
            Collider* collider = record.colliders[index];
            JPH::ShapeRefC leafShape = CreateLeafShape(*collider);
            if (!leafShape)
            {
                continue;
            }

            const Transform* colliderTransform = collider->transform();

            // ColliderのWorld姿勢をBodyのWorld姿勢で戻し、Compound Shape内の相対姿勢へ変換する。
            // 子GameObjectのColliderでも、この計算により親RigidbodyのBodyへ正しく配置される。
            const Vector3 relativePosition = inverseBodyRotation.RotateVector(
                collider->GetWorldCenter() - bodyPosition);
            const Quaternion relativeRotation = inverseBodyRotation
                * colliderTransform->GetRotation();
            settings.AddShape(
                JPH::Vec3(relativePosition.x, relativePosition.y, relativePosition.z),
                ToJoltRotation(relativeRotation),
                leafShape.GetPtr(),
                static_cast<uint32_t>(index + 1));
        }

        // Colliderが複数ある場合は、各Shapeの相対姿勢を保持したStaticCompoundShapeにまとめる。
        // AddShapeのuserDataにCollider配列の1始まりの番号を保存し、接触時の逆引きに使う。
        JPH::Shape::ShapeResult result = settings.Create();
        if (!result.IsValid())
        {
            Logger::Error("JoltのCompound Shape生成に失敗しました: " + result.GetError());
            return JPH::ShapeRefC(new JPH::EmptyShape());
        }
        return result.Get();
    }

    static JPH::EMotionType GetJoltMotionType(Rigidbody* rigidbody)
    {
        // RigidbodyがないColliderは静的Bodyとして生成する。
        if (!rigidbody)
        {
            return JPH::EMotionType::Static;
        }

        switch (rigidbody->GetMotionType())
        {
        case Rigidbody::MotionType::Static:
            return JPH::EMotionType::Static;
        case Rigidbody::MotionType::Kinematic:
            return JPH::EMotionType::Kinematic;
        case Rigidbody::MotionType::Dynamic:
        default:
            return JPH::EMotionType::Dynamic;
        }
    }

    static JPH::EAllowedDOFs GetAllowedDOFs(Rigidbody* rigidbody)
    {
        // 初期値では回転を固定し、Playerのような簡単な地上移動で転倒しないようにする。
        if (!rigidbody || !rigidbody->IsFreezeRotation())
        {
            return JPH::EAllowedDOFs::All;
        }
        return JPH::EAllowedDOFs::TranslationX
            | JPH::EAllowedDOFs::TranslationY
            | JPH::EAllowedDOFs::TranslationZ;
    }

    // -------------------------------------------------------------------------
    // Body生成と接触情報のキャッシュ
    // -------------------------------------------------------------------------

    JPH::BodyID CreateBody(BodyRecord& record)
    {
        // Bodyの生成時点でShapeと初期Transformを確定する。
        record.shape = CreateShape(record);

        const Transform* transform = record.bodyOwner ? record.bodyOwner->GetTransform() : nullptr;
        const Vector3 position = transform ? transform->GetPosition() : Vector3::Zero();
        const Quaternion rotation = transform ? transform->GetRotation() : Quaternion::Identity();
        const JPH::EMotionType motionType = GetJoltMotionType(record.rigidbody);

        // JoltのBody Layerは広域判定用の2値へ変換する。細かなLayer判定はListenerで行う。
        const JPH::ObjectLayer objectLayer = motionType == JPH::EMotionType::Static
            ? JoltPhysicsDetail::NON_MOVING_OBJECT_LAYER
            : JoltPhysicsDetail::MOVING_OBJECT_LAYER;

        JPH::BodyCreationSettings settings(
            record.shape.GetPtr(),
            ToJoltPosition(position),
            ToJoltRotation(rotation),
            motionType,
            objectLayer);

        // UserDataにはBodyRecordのアドレスを保存するが、現在の接触処理ではID索引を正としている。
        // JoltのBodyからゲームオブジェクトへ直接戻る経路を公開しないための保険として設定する。
        settings.mUserData = reinterpret_cast<uint64_t>(&record);
        settings.mAllowDynamicOrKinematic = true;
        settings.mCollideKinematicVsNonDynamic = true;
        settings.mAllowedDOFs = GetAllowedDOFs(record.rigidbody);

        if (record.rigidbody)
        {
            // Rigidbodyの値はBody生成時にJoltへ渡し、変更時はRefreshBodiesでBodyを再生成する。
            settings.mAllowSleeping = record.rigidbody->IsAllowSleep();
            settings.mFriction = record.rigidbody->GetFriction();
            settings.mRestitution = record.rigidbody->GetBounciness();
            settings.mGravityFactor = record.rigidbody->IsUseGravity() ? 1.0f : 0.0f;
            if (motionType == JPH::EMotionType::Dynamic)
            {
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
            return JPH::BodyID();
        }

        record.bodyID = bodyID;
        record.shapeSignature = CalculateShapeSignature(
            record.colliders,
            record.bodyOwner ? record.bodyOwner->GetTransform() : nullptr);
        record.settingsSignature = CalculateSettingsSignature(record.rigidbody);
        record.hasPhysicsTransform = true;
        record.lastPhysicsPosition = position;
        record.lastPhysicsRotation = rotation;

        if (record.rigidbody)
        {
            record.lastPhysicsVelocity = record.rigidbody->GetVelocity();
            record.hasPhysicsVelocity = true;
        }
        m_bodyRecordsByID[bodyID.GetIndexAndSequenceNumber()] = &record;
        return bodyID;
    }

    BodyRecord* FindBodyRecord(const JPH::BodyID& bodyID) const
    {
        // Callback中にBodyを直接保持せず、BodyIDの世代番号を含む値で安全に引き戻す。
        const auto iterator = m_bodyRecordsByID.find(bodyID.GetIndexAndSequenceNumber());
        return iterator != m_bodyRecordsByID.end() ? iterator->second : nullptr;
    }

    Collider* FindCollider(const BodyRecord& record, const JPH::SubShapeID& subShapeID) const
    {
        // Compound ShapeのuserDataはCreateShapeで設定したCollider配列の番号。
        // Colliderが1個だけの場合は、Joltが最適化してSubShapeIDを省略しても逆引きできる。
        if (record.colliders.empty())
        {
            return nullptr;
        }

        const uint64_t userData = record.shape
            ? record.shape->GetSubShapeUserData(subShapeID)
            : 0;
        if (userData > 0 && userData <= record.colliders.size())
        {
            return record.colliders[static_cast<size_t>(userData - 1)];
        }
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
        state.point = FromJoltPosition(point);
        // JoltのnormalはBody2をBody1から押し出す向きなので、
        // objectA側へ通知するnormalは反転する。
        state.normal = FromJoltVector(-manifold.mWorldSpaceNormal);
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

        result.objectA = result.colliderA ? result.colliderA->GetOwner() : nullptr;
        result.objectB = result.colliderB ? result.colliderB->GetOwner() : nullptr;
        result.rigidbodyA = result.colliderA ? result.colliderA->GetRigidbody() : nullptr;
        result.rigidbodyB = result.colliderB ? result.colliderB->GetRigidbody() : nullptr;
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
            if (!record || record->bodyID.IsInvalid() || !record->rigidbody)
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
                if (state.began && other && other->bodyOwner
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
        // Collider形状やRigidbody設定が変わった場合の簡易更新。
        // 既存Bodyを差し替えるため、速度とTransformを退避して新Bodyへ戻す。
        RemoveContactsForBody(record, events);

        Vector3 position = record.bodyOwner && record.bodyOwner->GetTransform()
            ? record.bodyOwner->GetTransform()->GetPosition()
            : Vector3::Zero();
        Quaternion rotation = record.bodyOwner && record.bodyOwner->GetTransform()
            ? record.bodyOwner->GetTransform()->GetRotation()
            : Quaternion::Identity();
        Vector3 velocity = record.rigidbody ? record.rigidbody->GetVelocity() : Vector3::Zero();

        DestroyBody(record);
        CreateBody(record);
        if (!record.bodyID.IsInvalid())
        {
            auto& bodyInterface = m_physicsSystem.GetBodyInterface();
            bodyInterface.SetPositionAndRotation(
                record.bodyID,
                ToJoltPosition(position),
                ToJoltRotation(rotation),
                JPH::EActivation::Activate);
            if (record.rigidbody && GetJoltMotionType(record.rigidbody) == JPH::EMotionType::Dynamic)
            {
                bodyInterface.SetLinearVelocity(record.bodyID, JPH::Vec3(velocity.x, velocity.y, velocity.z));
            }
            record.lastPhysicsPosition = position;
            record.lastPhysicsRotation = rotation;
            record.lastPhysicsVelocity = velocity;
        }
    }

    void CollectDesiredBodies(std::unordered_map<void*, DesiredBody>& desiredBodies)
    {
        // RigidbodyはColliderがなくてもEmptyShape Bodyとして登録する。
        // 登録済み集合には物理に関係するComponentしか入らないため、全Object走査は不要になる。
        for (Rigidbody* rigidbody : m_registeredRigidbodies)
        {
            if (!rigidbody || !rigidbody->IsEnabled() || !rigidbody->GetOwner())
            {
                continue;
            }

            GameObject* object = rigidbody->GetOwner();
            if (object->IsDestroyed() || !object->IsActive())
            {
                continue;
            }

            // Colliderがなくても候補を先に作る。CreateShapeがEmptyShapeを生成する。
            DesiredBody& body = desiredBodies[rigidbody];
            body.key = rigidbody;
            body.bodyOwner = object;
            body.rigidbody = rigidbody;
        }

        for (Collider* collider : m_registeredColliders)
        {
            if (!collider || !collider->IsEnabled() || !collider->GetOwner())
            {
                continue;
            }

            GameObject* object = collider->GetOwner();
            if (object->IsDestroyed() || !object->IsActive())
            {
                continue;
            }

            Rigidbody* ownerRigidbody = collider->GetRigidbody();
            if (ownerRigidbody && m_registeredRigidbodies.contains(ownerRigidbody)
                && ownerRigidbody->IsEnabled()
                && ownerRigidbody->GetOwner()->IsActive()
                && !ownerRigidbody->GetOwner()->IsDestroyed())
            {
                // 子Colliderは最も近い親RigidbodyのBodyへ追加する。
                DesiredBody& body = desiredBodies[ownerRigidbody];
                body.key = ownerRigidbody;
                body.bodyOwner = ownerRigidbody->GetOwner();
                body.rigidbody = ownerRigidbody;
                body.colliders.push_back(collider);
            }
            else if (!ownerRigidbody)
            {
                // RigidbodyのないColliderは、Colliderごとに独立したStatic Bodyとする。
                DesiredBody& body = desiredBodies[collider];
                body.key = collider;
                body.bodyOwner = object;
                body.colliders.push_back(collider);
            }
        }
    }

    std::unordered_map<void*, DesiredBody> BuildDesiredBodies()
    {
        // RefreshBodies中にm_bodyRecordsを直接変更しないよう、一時Mapへ候補を構築する。
        std::unordered_map<void*, DesiredBody> desiredBodies;
        CollectDesiredBodies(desiredBodies);
        return desiredBodies;
    }

    // -------------------------------------------------------------------------
    // 物理ステップ前後のTransform／速度同期
    // -------------------------------------------------------------------------

    void SyncBody(BodyRecord& record, float deltaTime, std::vector<DispatchEvent>& events)
    {
        if (record.bodyID.IsInvalid())
        {
            return;
        }

        // BodyのWorld移動・回転は識別子に使わない。相対形状だけを署名にしているため、
        // Dynamic Bodyが動いてもShapeを再生成せず、形状設定の変更だけを検出できる。
        const size_t shapeSignature = CalculateShapeSignature(
            record.colliders,
            record.bodyOwner ? record.bodyOwner->GetTransform() : nullptr);
        const size_t settingsSignature = CalculateSettingsSignature(record.rigidbody);
        if (shapeSignature != record.shapeSignature
            || settingsSignature != record.settingsSignature)
        {
            // Shape変更とBody設定変更は、現段階ではBody差し替えで扱う。
            RecreateBody(record, events);
            if (record.bodyID.IsInvalid())
            {
                return;
            }
        }

        const Transform* transform = record.bodyOwner ? record.bodyOwner->GetTransform() : nullptr;
        if (!transform)
        {
            return;
        }

        const Vector3 position = transform->GetPosition();
        const Quaternion rotation = transform->GetRotation();
        auto& bodyInterface = m_physicsSystem.GetBodyInterface();
        const JPH::EMotionType motionType = GetJoltMotionType(record.rigidbody);

        if (motionType == JPH::EMotionType::Kinematic)
        {
            // KinematicはScriptがTransformを正とする。移動速度はJolt側で接触判定に使用される。
            bodyInterface.MoveKinematic(
                record.bodyID,
                ToJoltPosition(position),
                ToJoltRotation(rotation),
                deltaTime);
        }
        else if (motionType == JPH::EMotionType::Static)
        {
            // Staticは必要な場合だけTransformを更新し、無用な起床を避ける。
            bodyInterface.SetPositionAndRotationWhenChanged(
                record.bodyID,
                ToJoltPosition(position),
                ToJoltRotation(rotation),
                JPH::EActivation::DontActivate);
        }
        else if (!record.hasPhysicsTransform
            || !NearlyEqual(position, record.lastPhysicsPosition)
            || !NearlyEqual(rotation, record.lastPhysicsRotation))
        {
            // Script側が前回の物理結果からTransformを変更した場合だけテレポートする。
            bodyInterface.SetPositionAndRotation(
                record.bodyID,
                ToJoltPosition(position),
                ToJoltRotation(rotation),
                JPH::EActivation::Activate);
        }

        if (record.rigidbody && motionType == JPH::EMotionType::Dynamic)
        {
            // ScriptがRigidbodyの速度を変更した場合だけJoltへ書き戻す。
            // 毎フレーム無条件にSetLinearVelocityすると、Joltの解決結果を上書きしてしまう。
            const Vector3 requestedVelocity = record.rigidbody->GetVelocity();
            if (!record.hasPhysicsVelocity
                || !NearlyEqual(requestedVelocity, record.lastPhysicsVelocity))
            {
                bodyInterface.SetLinearVelocity(
                    record.bodyID,
                    JPH::Vec3(requestedVelocity.x, requestedVelocity.y, requestedVelocity.z));
            }
        }
    }

    void RefreshBodies(float deltaTime, std::vector<DispatchEvent>& events)
    {
        // GameObjectの追加・削除、Componentの有効状態、親子関係の変更があったときだけ
        // 登録集合からBody候補を再構築する。通常Frameは既存Bodyから候補を再利用する。
        std::unordered_map<void*, DesiredBody> desiredBodies;
        if (m_structureDirty)
        {
            desiredBodies = BuildDesiredBodies();

            for (auto iterator = m_bodyRecords.begin(); iterator != m_bodyRecords.end();)
            {
                if (desiredBodies.find(iterator->first) == desiredBodies.end())
                {
                    // 既に存在しない候補は、接触終了を整理してからJoltからも削除する。
                    RemoveContactsForBody(*iterator->second, events);
                    DestroyBody(*iterator->second);
                    iterator = m_bodyRecords.erase(iterator);
                }
                else
                {
                    ++iterator;
                }
            }

            for (auto& [key, desired] : desiredBodies)
            {
                const auto existing = m_bodyRecords.find(key);
                if (existing == m_bodyRecords.end())
                {
                    // 新しいRigidbody／ColliderはこのFrameで初めてJoltへ登録する。
                    auto newRecord = std::make_unique<BodyRecord>();
                    newRecord->key = key;
                    newRecord->bodyOwner = desired.bodyOwner;
                    newRecord->rigidbody = desired.rigidbody;
                    newRecord->colliders = std::move(desired.colliders);
                    BodyRecord* record = newRecord.get();
                    m_bodyRecords.emplace(key, std::move(newRecord));
                    CreateBody(*record);
                }
                else
                {
                    // アドレスは同じでも、親子関係や形状、Rigidbody設定が変わる可能性がある。
                    BodyRecord* record = existing->second.get();
                    record->bodyOwner = desired.bodyOwner;
                    record->rigidbody = desired.rigidbody;
                    record->colliders = std::move(desired.colliders);
                }
            }

            m_structureDirty = false;
        }

        // 構成変更がないFrameでは、既存Bodyだけを直接同期する。
        for (auto& [key, recordPointer] : m_bodyRecords)
        {
            (void)key;
            SyncBody(*recordPointer, deltaTime, events);
        }
    }

    void SyncDynamicBodies()
    {
        // JoltのDynamic結果をゲーム側へ戻す。Transformの更新はこの1箇所に限定する。
        const auto& bodyInterface = m_physicsSystem.GetBodyInterfaceNoLock();
        for (auto& [key, recordPointer] : m_bodyRecords)
        {
            (void)key;
            BodyRecord& record = *recordPointer;
            if (!record.rigidbody || GetJoltMotionType(record.rigidbody) != JPH::EMotionType::Dynamic
                || record.bodyID.IsInvalid() || !record.bodyOwner || !record.bodyOwner->GetTransform())
            {
                continue;
            }

            // BodyのWorld値をTransformへ戻し、親を持つ場合は既存のWorld setterにLocal変換させる。
            JPH::RVec3 position;
            JPH::Quat rotation;
            bodyInterface.GetPositionAndRotation(record.bodyID, position, rotation);
            const Vector3 worldPosition = FromJoltPosition(position);
            const Quaternion worldRotation = FromJoltRotation(rotation);
            Transform* transform = record.bodyOwner->GetTransform();
            transform->SetPosition(worldPosition);
            transform->SetRotation(worldRotation);
            record.lastPhysicsPosition = worldPosition;
            record.lastPhysicsRotation = worldRotation;
            record.hasPhysicsTransform = true;

            // 次フレームにScriptが速度を変更したか比較できるよう、解決後の速度を保存する。
            const Vector3 velocity = FromJoltVector(bodyInterface.GetLinearVelocity(record.bodyID));
            record.rigidbody->SetVelocity(velocity);
            record.lastPhysicsVelocity = velocity;
            record.hasPhysicsVelocity = true;
        }
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
        // 1フレーム分の入力を受け取り、Body同期→固定刻み→結果同期の順で処理する。
        std::vector<DispatchEvent> events;
        RefreshBodies(deltaTime, events);

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
        m_registeredColliders.clear();
        m_registeredRigidbodies.clear();
        m_structureDirty = true;
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
        const uint64_t aObject = a.hitInfo.objectA ? a.hitInfo.objectA->GetId() : 0;
        const uint64_t bObject = b.hitInfo.objectA ? b.hitInfo.objectA->GetId() : 0;
        if (aObject != bObject)
        {
            return aObject < bObject;
        }
        const uint64_t aOther = a.hitInfo.objectB ? a.hitInfo.objectB->GetId() : 0;
        const uint64_t bOther = b.hitInfo.objectB ? b.hitInfo.objectB->GetId() : 0;
        return aOther < bOther;
    });

    for (const Backend::DispatchEvent& event : events)
    {
        // Destroy済みObjectへのcallbackは送らない。相手側のイベントは別のDispatchEventで処理される。
        GameObject* object = event.hitInfo.objectA;
        if (!object || object->IsDestroyed() || !object->IsActive())
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

void Physics::MarkDirty()
{
    if (s_backend)
    {
        s_backend->MarkDirty();
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
