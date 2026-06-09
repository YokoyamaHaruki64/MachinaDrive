#pragma once

#include "System/Component.h"
#include "System/GameObject.h"


class Player : public Component
{
private:
	Vector3 m_velocity{ 0.0f, 0.0f, 0.0f };
	bool m_isGrounded = false; // 地面に接地しているかどうかのフラグ

public:
    void Update() override;
    Player() = delete;
    Player(GameObject* owner) : Component(owner)
    {
    }
};
