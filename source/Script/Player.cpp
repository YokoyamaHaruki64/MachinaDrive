#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Player.h"
#include "System/FieldRegistry.h"

void Player::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_velocity);
    GM_FIELD(m_isGrounded);
}

#include "Scene/ResultScene.h"
#include "math/MathCommon.h"
#include "Script/Bullet.h"
#include "Script/Enemy.h"


void Player::OnStart()
{
    m_jumpSE = m_owner->GetComponent<Audio>();
}

void Player::Update()
{
    Camera* camera = Camera::GetMainCamera();
    Vector3 cameraDir = camera->GetTarget() - camera->transform()->GetPosition();
	cameraDir.y = 0.0f; // 水平方向のみに制限
	Vector3 cameraForward = cameraDir.Normalized();
	// カメラの右方向(既にForwardがxz平面になっているので転置によって高速化)
	Vector3 cameraRight = Vector3(cameraForward.z, 0.0f, -cameraForward.x); 

	const float moveSpeed = 50.0f;
	const float jumpForce = 5.0f;
	const float friction = 10.0f;

	Vector3 inputDirection{ 0.0f, 0.0f, 0.0f };
	if (Input::GetKeyPress(Input::KK_W))
	{
		inputDirection += cameraForward;
	}
	if( Input::GetKeyPress(Input::KK_S))
	{
		inputDirection -= cameraForward;
	}
	if( Input::GetKeyPress(Input::KK_A))
	{
		inputDirection -= cameraRight;
	}
	if( Input::GetKeyPress(Input::KK_D))
	{
		inputDirection += cameraRight;
	}
	
	if (Input::GetKeyTrigger(Input::KK_SPACE) && m_isGrounded)
	{
		m_velocity.y += jumpForce; // ジャンプ
		m_isGrounded = false; // ジャンプしたので地面から離れる
        if( m_jumpSE)
        {
            m_jumpSE->Play(); // ジャンプ音を再生
        }
	}

    if( Input::GetKeyPress(Input::KK_SHIFT))
    {
        inputDirection *= 2.0f; // ダッシュ
    }

    if( Input::GetKeyTrigger(Input::KK_F))
    {
        GameObject* bullet = Manager::Instantiate("Bullet",
            transform()->GetPosition() + transform()->GetForward() * 2.0f + Vector3(0.0f, 1.0f, 0.0f)
            , Vector3::Zero(), Vector3::One()); // 弾の生成
        bullet->GetComponent<Bullet>()->SetVelocity(transform()->GetForward() * 20.0f); // 弾の速度設定
    }

    m_velocity += inputDirection.Normalized() * moveSpeed * Time::GetDeltaTime(); // 入力に基づく加速度

	m_velocity.y -= 9.81f * Time::GetDeltaTime();					    // 重力
	m_velocity.x -= m_velocity.x * friction * Time::GetDeltaTime();	// 摩擦
	m_velocity.z -= m_velocity.z * friction * Time::GetDeltaTime();	// 摩擦

    transform()->Translate(m_velocity * Time::GetDeltaTime()); // 位置の更新
    float targetRotationY = atan2f(m_velocity.x, m_velocity.z); // 移動方向に向く
    const Vector3 myRot = transform()->GetRotation().QuatToEuler();
    transform()->SetRotation(Vector3(myRot.x, Math::LerpAngle(myRot.y, targetRotationY, 10.0f * Time::GetDeltaTime()), myRot.z)); // 回転の更新

	if(transform()->GetPosition().y < 0.0f)
	{
        transform()->SetPosition(Vector3(transform()->GetPosition().x, 0.0f, transform()->GetPosition().z)); // 地面に接地
		m_velocity.y = 0.0f; // 垂直方向の速度をリセット
		m_isGrounded = true;
	}

    auto enemies = Manager::GetObjectsByComponent<Enemy>();
    if(enemies.empty())
    {
        Manager::ChangeScene<ResultScene>();
    }
}
