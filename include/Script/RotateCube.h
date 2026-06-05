#pragma once

#include "System/Component.h"
#include "System/GameObject.h"

class RotateCube : public Component
{
    static constexpr float ROTATION_SPEED = 0.06f; // 回転速度の定数
public:
    RotateCube() = default;
    explicit RotateCube(GameObject* owner)
        : Component(owner)
    {
    }
    void Update() override
    {
        m_owner->GetTransform()->Rotate(Vector3(ROTATION_SPEED, ROTATION_SPEED, ROTATION_SPEED));
    }
};
