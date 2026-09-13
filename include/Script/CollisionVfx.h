#pragma once

#include "Common/Color.h"
#include "System/Core/Script.h"
#include "math/Curve.h"
#include "math/Vector3.h"

#include <cstdint>
#include <vector>

class ParticleEmitter;

// 接触点から衝突法線の両側へ火花を放つ一回限りのエフェクト。
class CollisionVfx final : public Script
{
    GM_SCRIPT(CollisionVfx, "Collision VFX")
    USE_FIELDS

    float m_coneAngle = 38.0f;
    float m_burstTime = 0.1f;
    int m_burstCount = 20;
    int m_maxParticles = 64;
    float m_particleLifetimeMin = 0.3f;
    float m_particleLifetimeMax = 0.6f;
    float m_speedMin = 30.0f;
    float m_speedMax = 60.0f;
    Vector3 m_gravity{ 0.0f, 6.0f, 0.0f };
    float m_drag = 0.8f;
    Vector3 m_particleSize{ 5.0f, 5.0f, 5.0f };
    Color m_orangeEmissionColor{ Color::Black() };
    Color m_yellowOrangeEmissionColor{ Color::Black() };
    Color m_redOrangeEmissionColor{ Color::Black() };
    Curve<float> m_scaleOverLifetime{
        {
            { 0.18f, 0.0f, 0.18f, 0.18f },
            { 1.0f, 0.10f, 1.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        }
    };
    CurveColor m_orangeColorOverLifetime{
        {
            { 1.0f, 0.0f, 1.0f, 1.0f },
            { 1.0f, 0.18f, 1.0f, 1.0f },
            { 0.65f, 1.0f, 0.65f, 0.6f },
        },
        {
            { 0.3f, 0.0f, 0.3f, 0.3f },
            { 0.75f, 0.18f, 0.75f, 0.75f },
            { 0.05f, 1.0f, 0.05f, 0.0f },
        },
        {
            { 0.04f, 0.0f, 0.04f, 0.04f },
            { 0.08f, 0.18f, 0.08f, 0.08f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        },
        {
            { 0.0f, 0.0f, 0.0f, 0.8f },
            { 1.0f, 0.08f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        }
    };
    CurveColor m_yellowOrangeColorOverLifetime{
        {
            { 1.0f, 0.0f, 1.0f, 1.0f },
            { 1.0f, 0.18f, 1.0f, 1.0f },
            { 0.85f, 1.0f, 0.85f, 0.7f },
        },
        {
            { 0.55f, 0.0f, 0.55f, 0.55f },
            { 1.0f, 0.18f, 1.0f, 1.0f },
            { 0.15f, 1.0f, 0.15f, 0.0f },
        },
        {
            { 0.04f, 0.0f, 0.04f, 0.04f },
            { 0.08f, 0.18f, 0.08f, 0.08f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        },
        {
            { 0.0f, 0.0f, 0.0f, 0.8f },
            { 1.0f, 0.08f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        }
    };
    CurveColor m_redOrangeColorOverLifetime{
        {
            { 1.0f, 0.0f, 1.0f, 1.0f },
            { 1.0f, 0.18f, 1.0f, 1.0f },
            { 0.55f, 1.0f, 0.55f, 0.45f },
        },
        {
            { 0.10f, 0.0f, 0.10f, 0.10f },
            { 0.45f, 0.18f, 0.45f, 0.45f },
            { 0.02f, 1.0f, 0.02f, 0.0f },
        },
        {
            { 0.0f, 0.0f, 0.0f, 0.0f },
            { 0.01f, 0.18f, 0.01f, 0.01f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        },
        {
            { 0.0f, 0.0f, 0.0f, 0.8f },
            { 1.0f, 0.08f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
        }
    };

    struct EmitterSlot
    {
        ParticleEmitter* emitter = nullptr;
        uint8_t colorVariant = 0;
    };
    std::vector<EmitterSlot> m_playerEmitters;
    std::vector<EmitterSlot> m_enemyEmitters;

    void ApplyEmitterSettings(
        ParticleEmitter* emitter,
        const Vector3& direction,
        const CurveColor& colorOverLifetime,
        const Color& emissionColor);
    const CurveColor& GetColorCurve(uint8_t colorVariant) const;
    const Color& GetEmissionColor(uint8_t colorVariant) const;

public:
    void OnStart() override;

    // HitInfoの法線はBからAを押し戻す方向なので、Player側はnormalを使う。
    void Configure(const Vector3& normal, const Vector3& contactPosition);
};
