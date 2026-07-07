#pragma once

#include "System/Component.h"

class PatternTrail : public Component
{
    std::vector<GameObject*> m_trailHeaders;

    bool m_isCreated = false;
    Vector3 m_normal = Vector3::Up();
public:
    PatternTrail() = delete;
    explicit PatternTrail(GameObject* owner)
        : Component(owner)
    {
    }
    virtual ~PatternTrail() {}
    virtual void Update() override;

    void CreateTrailPattern(
        const Vector3& position,
        const Vector3& normal,
        int trailCount);
};
