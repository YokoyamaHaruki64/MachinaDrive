#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/EnemyAnimation.h"

FIELDS_BEGIN(EnemyAnimation)
    GM_FIELD(m_floatHeight);
    GM_FIELD(m_floatSpeed);
FIELDS_END()

void EnemyAnimation::OnStart()
{
    Transform* modelTransform = transform();
    if (!modelTransform)
    {
        return;
    }

    m_initialLocalPosition = modelTransform->GetLocalPosition();
    m_elapsedTime = 0.0f;
}

void EnemyAnimation::Update()
{
    Transform* modelTransform = transform();
    if (!modelTransform)
    {
        return;
    }

    m_elapsedTime += Time::GetDeltaTime();

    const Vector3 floatingOffset(
        0.0f,
        std::sin(m_elapsedTime * m_floatSpeed) * m_floatHeight,
        0.0f);
    modelTransform->SetLocalPosition(m_initialLocalPosition + floatingOffset);
}
