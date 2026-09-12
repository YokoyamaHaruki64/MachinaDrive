#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/TimeRecoveryItem.h"

#include "Script/WaveDirector.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(TimeRecoveryItem)
    GM_FIELD(m_recoveryTime);
    GM_FIELD(m_lifetime);
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
}

void TimeRecoveryItem::Update()
{
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

    m_isCollected = true;
    m_owner->Destroy();
}
