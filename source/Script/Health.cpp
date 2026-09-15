#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/Health.h"

FIELDS_BEGIN(Health)
    GM_FIELD(m_maxHP);
FIELDS_END()

void Health::OnStart()
{
    m_maxHP = std::max(m_maxHP, 0.0f);
    m_currentHP = m_maxHP;
    m_isDead = false;
}

void Health::TakeDamage(float damage)
{
    if (m_isDead || damage <= 0.0f)
    {
        return;
    }

    const float appliedDamage = std::min(damage, m_currentHP);
    if (appliedDamage <= 0.0f)
    {
        return;
    }

    m_currentHP -= appliedDamage;
    for (const DamageCallback& callback : OnDamage)
    {
        if (callback)
        {
            callback(appliedDamage);
        }
    }

    if (m_currentHP > 0.0f)
    {
        return;
    }

    m_isDead = true;
    for (const DeathCallback& callback : OnDeath)
    {
        if (callback)
        {
            callback();
        }
    }
}
