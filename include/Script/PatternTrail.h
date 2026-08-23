#pragma once

#include "System/Script.h"
#include "System/GameObjectReference.h"

class EffectConfig;

class PatternTrail : public Script
{
    REGISTER_COMPONENT(PatternTrail, "PatternTrail");
    std::vector<GameObjectReference> m_trailHeaders;
    Vector3 m_normal = Vector3::Up();
    EffectConfig* m_config = nullptr;

    bool m_isCreated = false;
public:
    PatternTrail() = delete;
    explicit PatternTrail(GameObject* owner)
        : Script(owner)
    {
    }
    virtual ~PatternTrail() {}
    virtual void OnStart() override;
    virtual void Update() override;
    virtual void LateUpdate() override;

    void CreateTrailPattern(
        const Vector3& position,
        const Vector3& normal,
        int trailCount);
};
