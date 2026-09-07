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

public:
    void OnStart() override;
    void Update() override;
    void OnHitBegin(const HitInfo& hitInfo) override;
    void OnHit(const HitInfo& hitInfo) override;
    void OnHitEnd(const HitInfo& hitInfo) override;

    const Vector2 GetShotDirection()& { return m_accumulatedMouse; }
};
