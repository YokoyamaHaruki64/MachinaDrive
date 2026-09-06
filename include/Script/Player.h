#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

#include <unordered_set>

class Audio;
class Collider;
struct HitInfo;

class Player : public Script
{
    REGISTER_COMPONENT(Player, "Player");
private:
    // 旧シーンに残るフィールドとの互換用。実際の速度はRigidbodyを正とする。
    Vector3 m_velocity{ Vector3::Zero() };

    bool m_isGrounded = false;

    // 段差や複数Colliderに同時接触しても、1つが離れただけで空中判定にならないようにする。
    std::unordered_set<Collider*> m_groundColliders;

    Audio* m_jumpSE = nullptr;

public:
    void OnStart() override;
    void Update() override;
    void OnHitBegin(const HitInfo& hitInfo) override;
    void OnHit(const HitInfo& hitInfo) override;
    void OnHitEnd(const HitInfo& hitInfo) override;
    Player() = delete;
    Player(GameObject* owner) : Script(owner)
    {
    }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
