#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"
#include "Scene/ResultScene.h"
#include "math/MathCommon.h"
#include "Script/Bullet.h"
#include "Script/Enemy.h"
#include "Script/CameraController.h"

void Player::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_speed);
}

void Player::OnStart()
{
    m_jumpSE = GetComponent<Audio>();
    m_rigidbody = GetComponent<Rigidbody>();
    m_cameraController = Manager::GetFirstObjectByComponent<CameraController>()->GetComponent<CameraController>();

    if( !m_rigidbody )
    {
        Logger::Error("Player: Rigidbody component not found.");
    }

    if( !m_cameraController )
    {
        Logger::Error("Player: CameraController component not found.");
    }
}

void Player::Update()
{
    if (!m_rigidbody)
    {
        return;
    }

    Vector3 cameraForward = transform()->GetPosition() - m_cameraController->transform()->GetPosition();
    cameraForward.y = 0.0f;
    cameraForward.Normalize();
    Vector3 cameraRight = Vector3::Cross(Vector3::Up(), cameraForward);
    cameraRight.y = 0.0f;
    cameraRight.Normalize();

    if (!Input::IsCursorVisible())
    {
        if (Input::GetMouseTrigger(Input::MouseButton::Left))
        {
            m_cameraController->BeginLock();
            Input::SetMousePositionNormalized({ 0.5f, 0.5f });
            m_rigidbody->SetVelocity(Vector3::Zero());
            m_accumulatedMouse = Vector2::Zero();
        }
        if (Input::GetMousePress(Input::MouseButton::Left))
        {
            m_accumulatedMouse += Input::GetMouseRawDelta();

            Vector3 move = cameraForward * m_accumulatedMouse.y + cameraRight * -m_accumulatedMouse.x;
        }
        if (Input::GetMouseRelease(Input::MouseButton::Left))
        {
            Vector3 move = cameraForward * m_accumulatedMouse.y + cameraRight * -m_accumulatedMouse.x;

            if (move.LengthSqr() > 0.0f)
            {
                move.Normalize();
                m_rigidbody->SetVelocity(move * m_speed);
            }
            m_cameraController->EndLock();
        }
    }
}

void Player::OnHitBegin(const HitInfo& hitInfo)
{
    if (!hitInfo.hit || hitInfo.isTrigger || hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

    // PhysicsはA側から見た押し戻し方向をnormalに入れるため、上向きなら足元の接触である。
    if (hitInfo.normal.y > 0.5f)
    {
    }
}

void Player::OnHit(const HitInfo& hitInfo)
{
    if (!hitInfo.hit || hitInfo.isTrigger || hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

    if (hitInfo.normal.y > 0.5f)
    {
    }
}

void Player::OnHitEnd(const HitInfo& hitInfo)
{
    if (hitInfo.objectA != m_owner || !hitInfo.colliderB)
    {
        return;
    }

}
