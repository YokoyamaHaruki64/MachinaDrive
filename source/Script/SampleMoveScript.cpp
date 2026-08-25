#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/SampleMoveScript.h"

FIELDS_BEGIN(SampleMoveScript)
    GM_FIELD(m_settings);
    GM_FIELD(m_lookTarget);
    GM_FIELD(m_spawnPrefab);
FIELDS_END()

void SampleMoveScript::OnStart()
{
    m_audio = GetComponent<Audio>();
    Logger::Log("SampleMoveScript started: " + GetOwner()->GetName());
}

Vector3 SampleMoveScript::BuildMoveDirection() const
{
    Vector3 direction = Vector3::Zero();
    if (Input::GetKeyPress(Input::KK_W))
    {
        direction += transform()->GetForward();
    }
    if (Input::GetKeyPress(Input::KK_S))
    {
        direction -= transform()->GetForward();
    }
    if (Input::GetKeyPress(Input::KK_D))
    {
        direction += transform()->GetRight();
    }
    if (Input::GetKeyPress(Input::KK_A))
    {
        direction -= transform()->GetRight();
    }

    return direction.Normalized();
}

void SampleMoveScript::Update()
{
    const Vector3 direction = BuildMoveDirection();
    float moveSpeed = m_settings.GetMoveSpeed();
    if (m_settings.IsDashAllowed() && Input::GetKeyPress(Input::KK_SHIFT))
    {
        moveSpeed *= m_settings.GetDashMultiplier();
    }

    transform()->Translate(direction * (moveSpeed * Time::GetDeltaTime()));

    if (Input::GetKeyTrigger(Input::KK_F) && m_spawnPrefab)
    {
        const Vector3 spawnPosition = transform()->GetPosition() + m_settings.GetSpawnOffset();
        GameObject* spawnedObject = Manager::Instantiate(
            m_spawnPrefab.GetPath(),
            spawnPosition,
            Vector3::Zero(),
            Vector3::One());
        if (!spawnedObject)
        {
            Logger::Warning("SampleMoveScript failed to instantiate: " + m_spawnPrefab.GetPath());
        }
        else if (m_audio)
        {
            m_audio->Play();
        }
    }
}

void SampleMoveScript::LateUpdate()
{
    if (!m_lookTarget)
    {
        return;
    }

    const Vector3 direction = m_lookTarget->GetTransform()->GetPosition() - transform()->GetPosition();
    if (direction.LengthSqr() > 0.0f)
    {
        transform()->SetRotation(Quaternion::LookRotation(direction));
    }
}
