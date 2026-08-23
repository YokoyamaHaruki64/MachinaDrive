#pragma once

#include "System/Script.h"
#include "System/GameObject.h"

class Audio;
class Player : public Script
{
    REGISTER_COMPONENT(Player, "Player");
private:
	Vector3 m_velocity{ 0.0f, 0.0f, 0.0f };
	bool m_isGrounded = false; // 地面に接地しているかどうかのフラグ
    Audio* m_jumpSE = nullptr;

public:
    void OnStart() override;
    void Update() override;
    Player() = delete;
    Player(GameObject* owner) : Script(owner)
    {
    }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
