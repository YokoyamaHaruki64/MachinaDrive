#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"
#include "Scene/ResultScene.h"
#include "Script/Enemy.h"
#include "Script/CameraController.h"
#include "Script/PlayerAnimation.h"
#include "Script/Health.h"
#include "Script/PostProcessImpulse.h"
#include "Script/UI/GameHud.h"
#include "Script/Pause.h"

FIELDS_BEGIN(Player)
    GM_FIELD(m_speed);
    GM_FIELD(m_attackTime);
    GM_FIELD(m_attackEndDuration);
    GM_FIELD(m_speedFriction);
    GM_FIELD(m_attackReadyVfxPrefab);
    GM_FIELD(m_playerLaunchVfxPrefab);
    GM_FIELD(m_attackDashTrailPrefab);
    GM_FIELD(m_debug);
FIELDS_END()

void Player::OnStart()
{
    // Player操作はマウスを相対移動で使うため、カーソルを捕捉する。
    Input::SetCursorVisible(false);
    Input::SetCursorLock(true);

    m_chargeAudio = m_owner->AddComponent<Audio>("Assets/Local/Sounds/SE_Charge_Loop.wav");
    m_shotAudio = m_owner->AddComponent<Audio>("Assets/Local/Sounds/SE_Shot.wav");
    m_shotAudio->SetGain(5.0f);
    m_rigidbody = GetComponent<Rigidbody>();
    m_health = GetComponent<Health>();
    m_cameraController = Manager::GetFirstObjectByComponent<CameraController>()->GetComponent<CameraController>();

    for (auto* childTransform : transform()->GetChildren())
    {
        GameObject* childObject = childTransform ? childTransform->GetOwner() : nullptr;
        m_playerAnimation = childObject ? childObject->GetComponent<PlayerAnimation>() : nullptr;
        if (m_playerAnimation)
        {
            break;
        }
    }

    if (GameObject* hudObject = Manager::GetFirstObjectByComponent<GameHud>())
    {
        m_gameHud = hudObject->GetComponent<GameHud>();
    }

    if( !m_rigidbody )
        Logger::Error("Player: Rigidbody component not found.");

    if (!m_health)
        Logger::Error("Player: Health component not found.");

    if( !m_cameraController )
        Logger::Error("Player: CameraController component not found.");

    if (!m_playerAnimation)
        Logger::Error("Player: PlayerAnimation component not found on a child Model.");

    if (!m_gameHud)
        Logger::Error("Player: GameHud component not found.");

    if (m_gameHud)
    {
        m_gameHud->SetDriveGauge(1.0f);
        m_gameHud->SetDriveReady(true);
    }

    if (m_health)
    {
        if (m_gameHud)
        {
            m_gameHud->SetHealth(m_health->GetCurrentHP(), m_health->GetMaxHP());
        }

        m_health->OnDamage.emplace_back([this](float damage)
        {
            if (m_gameHud)
            {
                m_gameHud->ApplyHealthDamage(damage);
            }
        });

        m_health->OnDamage.emplace_back([this](float)
        {
            if(m_cameraController)
            {
                m_cameraController->Shake(0.3f, 0.2f);
            }
        });

    }

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
    if (Input::GetKeyTrigger(Input::KeyCode::E))
    {
        Vector3 playerForward = transform()->GetForward();
        Manager::Instantiate("Assets/Prefabs/Enemy.prefab",
            playerForward * 20.0f + transform()->GetPosition(),
            Quaternion::Identity().QuatToEuler(),
            Vector3::One());

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
        GameObject* model = transform()->GetChildren()[1]->GetOwner();
        MaterialPropertyOverride& debugMaterial = model->GetComponent<ModelRenderer>()->GetMaterialPropertyOverride(0);
        debugMaterial.SetColor("_BaseColor", m_debugColor[m_state]);

        if(Input::GetKeyTrigger(Input::KeyCode::G))
        {
            if (m_health)
            {
                m_health->TakeDamage(10.0f);
            }
        }

        if(Input::GetKeyTrigger(Input::KeyCode::P))
        {
            Pause::SetPause(!Pause::IsPause());
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

        if (m_chargeAudio)
        {
            m_chargeAudio->Play(true);
        }

        if (m_playerAnimation)
        {
            m_playerAnimation->StartAttackReadyAnimation();
        }

        m_cameraController->BeginLock();
        Input::SetMousePositionNormalized({ 0.5f, 0.5f });
        m_rigidbody->SetVelocity(Vector3::Zero());
        m_accumulatedMouse = Vector2::Zero();

        m_director = Manager::Instantiate("Assets/Prefabs/ShotDirector.prefab");

        if (m_attackReadyVfxPrefab)
        {
            m_attackReadyVfx = Manager::Instantiate(
                m_attackReadyVfxPrefab.GetPath(),
                transform()->GetPosition(),
                Vector3::Zero(),
                Vector3::One());
        }
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
        if (m_chargeAudio)
        {
            m_chargeAudio->Stop();
        }
        if (m_shotAudio)
        {
            m_shotAudio->Play();
        }

        m_state = Attacking;
        m_attackTimer = (std::max)(m_attackTime, 0.0f);
        m_attackEndTimer = (std::max)(m_attackEndDuration, 0.0f);
        if (m_gameHud)
        {
            m_gameHud->SetDriveGauge(0.0f);
            m_gameHud->SetDriveReady(false);
        }

        if (m_playerAnimation)
        {
            m_playerAnimation->StartAttackAnimation();
        }

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

        if (m_attackReadyVfx.Get())
        {
            m_attackReadyVfx->Destroy();
            m_attackReadyVfx.Clear();
        }

        PostProcessImpulse::Play("PlayerShot");

        // 発進の瞬間と移動中の軌跡を分け、開始方向を見失わないようにする。
        if (m_playerLaunchVfxPrefab)
        {
            Manager::Instantiate(
                m_playerLaunchVfxPrefab.GetPath(),
                transform()->GetPosition(),
                Vector3::Zero(),
                Vector3::One());
        }

        if (m_attackDashTrailPrefab)
        {
            Manager::Instantiate(
                m_attackDashTrailPrefab.GetPath(),
                transform()->GetPosition(),
                Vector3::Zero(),
                Vector3::One());
        }
    }
}

void Player::UpdateAttacking()
{
    m_attackTimer -= Time::GetDeltaTime();
    if (m_attackTimer <= 0.0f)
    {
        m_attackTimer = 0.0f;
        m_attackEndTimer = (std::max)(m_attackEndDuration, 0.0f);
        m_state = AttackEnd;
        UpdateAttackProgress();
        if (m_gameHud)
        {
            m_gameHud->ResetCombo();
        }
        return;
    }

    UpdateAttackProgress();
    float damping = std::exp(-m_speedFriction * Time::GetDeltaTime());

    m_rigidbody->SetVelocity(m_rigidbody->GetVelocity() * damping);
}

void Player::UpdateAttackEnd()
{
    m_attackEndTimer -= Time::GetDeltaTime();
    UpdateAttackProgress();

    Vector3 velocity = m_rigidbody->GetVelocity();

    if (m_attackEndTimer > 0.0f)
    {
        m_rigidbody->SetVelocity(Math::Lerp(Vector3::Zero(), velocity, 0.95f));
    }
    else
    {
        m_rigidbody->SetVelocity(Vector3::Zero());
        m_state = Idle;
        if (m_gameHud)
        {
            m_gameHud->SetDriveGauge(1.0f);
            m_gameHud->SetDriveReady(true);
        }
    }
}

void Player::UpdateAttackProgress()
{
    if (!m_gameHud)
    {
        return;
    }

    const float attackDuration = (std::max)(m_attackTime, 0.0f);
    const float attackEndDuration = (std::max)(m_attackEndDuration, 0.0f);
    const float totalDuration = attackDuration + attackEndDuration;
    const float attackElapsed = attackDuration - (std::max)(m_attackTimer, 0.0f);
    const float attackEndElapsed = attackEndDuration - (std::max)(m_attackEndTimer, 0.0f);
    const float elapsed = attackElapsed + attackEndElapsed;
    const float progress = totalDuration > 0.0f ? elapsed / totalDuration : 1.0f;

    m_gameHud->SetDriveGauge(progress);
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
