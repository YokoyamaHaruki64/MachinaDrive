#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"
#include "Scene/ResultScene.h"
#include "Script/Bullet.h"
#include "Script/Enemy.h"
#include "Script/CameraController.h"
#include "Script/FillBar.h"
#include "Script/ComboUI.h"

FIELDS_BEGIN(Player)
    GM_FIELD(m_hpBarObject);
    GM_FIELD(m_comboUIObject);
    GM_FIELD(m_speed);
    GM_FIELD(m_attackTime);
    GM_FIELD(m_speedFriction);
    GM_FIELD(m_debug);
FIELDS_END()

void Player::OnStart()
{
    m_jumpSE = GetComponent<Audio>();
    m_rigidbody = GetComponent<Rigidbody>();
    m_cameraController = Manager::GetFirstObjectByComponent<CameraController>()->GetComponent<CameraController>();

    m_hpBar = m_hpBarObject.Get() ? m_hpBarObject->GetComponent<FillBar>() : nullptr;
    m_comboUI = m_comboUIObject.Get() ? m_comboUIObject->GetComponent<ComboUI>() : nullptr;

    if( !m_rigidbody )
        Logger::Error("Player: Rigidbody component not found.");

    if( !m_cameraController )
        Logger::Error("Player: CameraController component not found.");

    if( !m_hpBar )
        Logger::Error("Player: FillBar component not found in the specified HP Bar GameObject.");

    if (!m_comboUI)
        Logger::Error("Player: ComboUI component not found in the specified Combo UI GameObject.");

#ifndef _DEBUG
    m_debug = false;
#endif // !_DEBUG
}

void Player::Update()
{
    if (!m_rigidbody)
    {
        return;
    }

    if(m_isHitStop)
    {
        m_hitStopTimer -= Time::GetDeltaTime();
        if( m_hitStopTimer <= 0.0f )
        {
            m_isHitStop = false;
            m_rigidbody->SetEnabled(true);
        }
        return;
    }

    Vector3 velocity = m_rigidbody->GetVelocity();
    m_rigidbody->SetVelocity({ velocity.x, 0.0f, velocity.z });

    switch(m_state)
    {
        case Idle:
            UpdateIdle();
            break;

        case AttackReady:
            UpdateAttackReady();
            break;

        case Attacking:
            UpdateAttacking();
            break;

        case AttackEnd:
            UpdateAttackEnd();
            break;
    }

    if (m_debug)
    {
        MaterialPropertyOverride& debugMaterial = GetComponent<ModelRenderer>()->GetMaterialPropertyOverride(1);
        debugMaterial.SetColor("_BaseColor", m_debugColor[m_state]);

        if(Input::GetKeyTrigger(Input::KeyCode::G))
        {
            m_hpBar->AddCurrent(-10.0f);
        }

    }
}

void Player::UpdateIdle()
{
    Vector3 cameraForward;
    Vector3 cameraRight;

    if (!GetCameraDirection(cameraForward, cameraRight)) return;

    if (Input::IsCursorVisible()) return;

    if (Input::GetMouseTrigger(Input::MouseButton::Left))
    {
        m_state = AttackReady;

        m_cameraController->BeginLock();
        Input::SetMousePositionNormalized({ 0.5f, 0.5f });
        m_rigidbody->SetVelocity(Vector3::Zero());
        m_accumulatedMouse = Vector2::Zero();

        m_director = Manager::Instantiate("Assets/Prefabs/ShotDirector.prefab");
    }

}

void Player::UpdateAttackReady()
{
    Vector3 cameraForward;
    Vector3 cameraRight;

    if (!GetCameraDirection(cameraForward, cameraRight)) return;

    if (Input::GetMousePress(Input::MouseButton::Left))
    {
        m_accumulatedMouse += Input::GetMouseRawDelta();

        Vector3 move = cameraForward * m_accumulatedMouse.y + cameraRight * -m_accumulatedMouse.x;
    }

    if (Input::GetMouseRelease(Input::MouseButton::Left))
    {
        m_state = Attacking;
        m_attackTimer = m_attackTime;

        Vector3 move = cameraForward * m_accumulatedMouse.y + cameraRight * -m_accumulatedMouse.x;

        if (move.LengthSqr() > 0.0f)
        {
            move.Normalize();
            m_rigidbody->SetVelocity(move * m_speed);
        }
        m_cameraController->EndLock();
        if (m_director.Get())
        {
            m_director->Destroy();
            m_director.Clear();
        }
    }
}

void Player::UpdateAttacking()
{
    m_attackTimer -= Time::GetDeltaTime();
    if (m_attackTimer <= 0.0f)
    {
        m_state = AttackEnd;
        m_comboUI->ResetCombo();
        return;
    }
    float damping = std::exp(-m_speedFriction * Time::GetDeltaTime());

    m_rigidbody->SetVelocity(m_rigidbody->GetVelocity() * damping);
}

void Player::UpdateAttackEnd()
{
    Vector3 velocity = m_rigidbody->GetVelocity();

    if (velocity.LengthSqr() > 1.0f)
    {
        m_rigidbody->SetVelocity(Math::Lerp(Vector3::Zero(), velocity, 0.95f));
    }
    else
    {
        m_rigidbody->SetVelocity(Vector3::Zero());
        m_state = Idle;
    }


}

bool Player::GetCameraDirection(Vector3& forward, Vector3& right) const
{
    if (!m_cameraController) false;

    forward = transform()->GetPosition() - m_cameraController->transform()->GetPosition();
    forward.y = 0.0f;
    forward.Normalize();
    right = Vector3::Cross(Vector3::Up(), forward);
    right.y = 0.0f;
    right.Normalize();

    return true;
}

void Player::OnHitBegin(const HitInfo& hitInfo)
{

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

void Player::HitStop(float duration)
{
    if (m_rigidbody)
    {
        m_isHitStop = true;
        m_hitStopTimer = duration;
        m_rigidbody->SetEnabled(false);
    }
}
