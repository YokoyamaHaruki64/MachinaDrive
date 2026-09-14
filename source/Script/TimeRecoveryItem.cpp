#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/TimeRecoveryItem.h"

#include "Script/WaveDirector.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(TimeRecoveryItem)
    GM_FIELD(m_recoveryTime);
    GM_FIELD(m_lifetime);
    GM_FIELD(m_idleVfxPrefab);
    GM_FIELD(m_pickupVfxPrefab);
    GM_FIELD(m_amplitude);
    GM_FIELD(m_frequency);
FIELDS_END()

void TimeRecoveryItem::OnStart()
{
    if (GameObject* waveDirectorObject = Manager::GetFirstObjectByComponent<WaveDirector>())
    {
        m_waveDirector = waveDirectorObject->GetComponent<WaveDirector>();
    }

    m_recoveryTime = Math::Max(0.0f, m_recoveryTime);
    m_lifetime = Math::Max(0.0f, m_lifetime);
    m_elapsedTime = 0.0f;
    m_isCollected = false;
    m_initialPosition = transform()->GetPosition();

    if (m_idleVfxPrefab)
    {
        m_idleVfx = Manager::Instantiate(
            m_idleVfxPrefab.GetPath(),
            transform()->GetPosition(),
            Vector3::Zero(),
            Vector3::One());
    }


}

void TimeRecoveryItem::Update()
{

    const float verticalOffset = m_amplitude * std::sin(m_frequency * m_elapsedTime * Math::PI_2);
    transform()->SetPosition(m_initialPosition + Vector3(0.0f, verticalOffset, 0.0f));

    if (m_idleVfx)
    {
        m_idleVfx->GetTransform()->SetPosition(transform()->GetPosition());
    }

    m_elapsedTime += Time::GetDeltaTime();

    if (m_elapsedTime >= m_lifetime)
    {
        m_owner->Destroy();
    }
}

void TimeRecoveryItem::OnHitBegin(const HitInfo& hitInfo)
{
    if (m_isCollected || hitInfo.objectA != m_owner || !hitInfo.objectB || hitInfo.objectB->GetTag() != "Player")
    {
        return;
    }

    if (m_waveDirector)
    {
        m_waveDirector->AddRemainingTime(m_recoveryTime);
    }

    DestroyIdleVfx();

    if (m_pickupVfxPrefab && hitInfo.objectB->GetTransform())
    {
        // 取得者の周囲から上昇するプラス記号で、時間回復をワールド上でも通知する。
        const Vector3 vfxPosition = hitInfo.objectB->GetTransform()->GetPosition() + Vector3::Up() * 1.0f;
        Manager::Instantiate(m_pickupVfxPrefab.GetPath(), vfxPosition, Vector3::Zero(), Vector3::One());
    }

    m_isCollected = true;
    m_owner->Destroy();
}

void TimeRecoveryItem::OnDestroy()
{
    DestroyIdleVfx();
}

void TimeRecoveryItem::DestroyIdleVfx()
{
    if (m_idleVfx)
    {
        m_idleVfx->Destroy();
        m_idleVfx.Clear();
    }
}
