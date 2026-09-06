#pragma once

#include "Component/Collider.h"
#include "math/Vector3.h"

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

class GameObject;
class Rigidbody;

// 物理イベントは常にobjectA側から見た情報として通知する。
// normalはBからAを押し戻す方向で、A/Bを入れ替えた通知では反転する。
struct HitInfo
{
    bool hit{ false };
    bool isTrigger{ false };

    // 接触点、A側から見た押し戻し方向、めり込み深度。
    Vector3 point{ Vector3::Zero() };
    Vector3 normal{ Vector3::Zero() };
    float penetration{ 0.0f };

    // Aは通知を受ける側、Bは相手側。Colliderが子Objectにある場合は子Objectが入る。
    Collider* colliderA{ nullptr };
    Collider* colliderB{ nullptr };
    GameObject* objectA{ nullptr };
    GameObject* objectB{ nullptr };

    // Colliderから親方向へ辿って解決したRigidbody。存在しない場合はnullptr。
    Rigidbody* rigidbodyA{ nullptr };
    Rigidbody* rigidbodyB{ nullptr };
};

// Joltの世界と接触状態を管理する。PlayerやEnemyはこのクラスから参照しない。
class Physics final
{
    struct Backend;
    static std::unique_ptr<Backend> s_backend;

public:
    using CollisionLayer = uint8_t;

    static constexpr CollisionLayer DefaultLayer = 0;
    static constexpr CollisionLayer PlayerLayer = 1;
    static constexpr CollisionLayer EnemyLayer = 2;
    static constexpr CollisionLayer MaxLayer = 8;

    // Layer同士の衝突可否はここへ列挙する。ペアの順番は区別しない。
    static constexpr std::array<std::pair<CollisionLayer, CollisionLayer>, 4> CollisionPairs{
        std::pair{ DefaultLayer, DefaultLayer },
        std::pair{ DefaultLayer, PlayerLayer },
        std::pair{ DefaultLayer, EnemyLayer },
        std::pair{ PlayerLayer, EnemyLayer },
    };

    // Managerの初期化／終了から一度ずつ呼び出す。
    static bool Initialize();
    static void Uninitialize();

    // 全ComponentのUpdate後に呼び出し、必要なBodyを同期して固定物理ステップを進める。
    static void Update(float deltaTime);

    // 物理対象Componentの生存期間をPhysicsへ伝える。
    // PhysicsはManagerの全GameObjectを毎Step走査せず、登録済みComponentだけを管理する。
    static void RegisterCollider(Collider* collider);
    static void UnregisterCollider(Collider* collider);
    static void RegisterRigidbody(Rigidbody* rigidbody);
    static void UnregisterRigidbody(Rigidbody* rigidbody);

    // Component側から変更内容を明示的に通知する。
    // Physicsはこれらの通知を受けたBodyだけを次のUpdateで更新し、通常Frameに値を比較しない。
    static void MarkBodyShapeDirty(Collider* collider);
    static void MarkBodyShapeDirty(Rigidbody* rigidbody);
    static void MarkBodySettingsDirty(Rigidbody* rigidbody, bool requiresBodyRecreate);
    static void MarkBodyVelocityDirty(Rigidbody* rigidbody);
    static void MarkBodyTransformDirty(Collider* collider);
    static void MarkBodyTransformDirty(Rigidbody* rigidbody);
    static void RebindCollider(Collider* collider);
    static void RefreshRigidbodyState(Rigidbody* rigidbody);

    // Active変更は既存のGameObject lifecycleから通知する。
    static void NotifyGameObjectStateChanged(GameObject* object);

    // SceneのGameObjectを破棄する前に、Bodyと接触キャッシュを通知なしで破棄する。
    static void Clear();

    static bool IsInitialized();

    // Compile-timeのLayer組み合わせ表を使って衝突可否を返す。
    static bool CanLayersCollide(CollisionLayer layerA, CollisionLayer layerB);
};
