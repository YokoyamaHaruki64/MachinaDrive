#include "Script/Player.h"

#include "System/input.h"
#include "Component/Transform.h"
#include "Component/Camera.h"
#include "math/MathCommon.h"


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
	const float dt = 1.0f / 60.0f; // 仮のデルタタイム（60FPS想定）

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
	}

	m_velocity += inputDirection.Normalized() * moveSpeed * dt; // 加速度

	m_velocity.y -= 9.81f * dt;					    // 重力
	m_velocity.x -= m_velocity.x * friction * dt;	// 摩擦
	m_velocity.z -= m_velocity.z * friction * dt;	// 摩擦

    m_transform->Translate(m_velocity * dt); // 位置の更新
    float targetRotationY = atan2f(m_velocity.x, m_velocity.z); // 移動方向に向く
    const Vector3& myRot = m_transform->GetRotation();
    m_transform->SetRotation(Vector3(myRot.x, Math::LerpAngle(myRot.y, targetRotationY, 10.0f * dt), myRot.z)); // 回転の更新

	if(m_transform->GetPosition().y < 0.0f)
	{
        m_transform->SetPosition(Vector3(m_transform->GetPosition().x, 0.0f, m_transform->GetPosition().z)); // 地面に接地
		m_velocity.y = 0.0f; // 垂直方向の速度をリセット
		m_isGrounded = true;
	}
}
