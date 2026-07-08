#pragma once

#include "System/Component.h"

class PatternTrailManager;

class PatternTrail : public Component
{
    std::vector<GameObject*> m_trailHeaders;
    Vector3 m_normal = Vector3::Up();
    PatternTrailManager* m_manager = nullptr;

    bool m_isCreated = false;
public:
    PatternTrail() = delete;
    explicit PatternTrail(GameObject* owner)
        : Component(owner)
    {
    }
    virtual ~PatternTrail() {}
    virtual void OnStart() override;
    virtual void Update() override;

    void CreateTrailPattern(
        const Vector3& position,
        const Vector3& normal,
        int trailCount);
};
