#pragma once

#include "System/Component.h"
#include "System/GameObject.h"


class Player : public Component
{
private:
	Vector3 m_velocity{ 0.0f, 0.0f, 0.0f };
    Transform* m_transform = nullptr;
	bool m_isGrounded = false; // 地面に接地しているかどうかのフラグ

public:
    void OnStart() override
    {
        m_transform = m_owner->GetTransform();
    }
    void Update() override;
    Player() = delete;
    Player(GameObject* owner) : Component(owner)
    {
    }
};
