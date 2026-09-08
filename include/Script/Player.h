#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

#include <unordered_set>

class Audio;
class Collider;
struct HitInfo;
class CameraController;

class Player : public Script
{
    GM_SCRIPT(Player, "Player");
    USE_FIELDS

private:
    Rigidbody* m_rigidbody = nullptr;
    CameraController* m_cameraController = nullptr;
    GameObjectReference m_director;

    Audio* m_jumpSE = nullptr;

    Vector2 m_beginMousePos{ Vector2::Zero() };
    Vector2 m_accumulatedMouse{ Vector2::Zero() };
    
    float m_speed = 10.0f;
    float m_attackTime = 5.0f;
    float m_speedFriction = 0.9f;

    float m_attackTimer = 0.0f;

    bool m_isHitStop = false;
    float m_hitStopTimer = 0.0f;

    enum AttackState
    {
        Idle,
        AttackReady,
        Attacking,
        AttackEnd,

        Count
    };
    AttackState m_state = Idle;

    void UpdateIdle();
    void UpdateAttackReady();
    void UpdateAttacking();
    void UpdateAttackEnd();

    Color m_debugColor[Count] = {
        Color(0.0f, 1.0f, 0.0f, 1.0f), // Idle
        Color(1.0f, 1.0f, 0.0f, 1.0f), // AttackReady
        Color(1.0f, 0.0f, 0.0f, 1.0f), // Attacking
        Color(0.5f, 0.5f, 0.5f, 1.0f)  // AttackEnd
    };
    bool m_debug;

    bool GetCameraDirection(Vector3& forward, Vector3& right) const;
public:
    void OnStart() override;
    void Update() override;
    void OnHitBegin(const HitInfo& hitInfo) override;
    void OnHit(const HitInfo& hitInfo) override;
    void OnHitEnd(const HitInfo& hitInfo) override;

    void HitStop(float duration);

    const Vector2 GetShotDirection()& { return m_accumulatedMouse; }
};
