#pragma once

#include "System/Component.h"

class Quad;

class Ripple : public Component
{
    Quad* m_quad = nullptr;

    float m_lifeTime = 1.0f; // 波紋の寿命
    float m_elapsedTime = 0.0f; // 経過時間
public:
    Ripple(GameObject* owner) :Component(owner)
    {}
    virtual ~Ripple() override {}
    virtual void OnStart() override;
    virtual void Update() override;

    void SetLifeTime(float lifeTime) { m_lifeTime = lifeTime; }
    const float GetLifeTime() const { return m_lifeTime; }
};
