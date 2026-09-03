#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

class Audio;
class Player : public Script
{
    REGISTER_COMPONENT(Player, "Player");
private:
	Vector3 m_velocity{ 0.0f, 0.0f, 0.0f };
	bool m_isGrounded = false; // 地面に接地しているかどうかのフラグ
    // Physics::CheckCollisionの結果を前フレームと比較し、接触開始・終了時だけ通知する。
    // 毎フレームログを出すと判定自体の確認が難しくなるため、状態だけを保持する。
    bool m_isTouchingEnemy = false;
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
