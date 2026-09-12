#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Random.h"
#include "Script/AttackReadyVfx.h"
#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>

FIELDS_BEGIN(AttackReadyVfx)
    GM_FIELD(m_headPrefab);
    GM_FIELD(m_spawnInterval);
    GM_FIELD(m_maxActiveHeads);
    GM_FIELD(m_spawnRadiusMin);
    GM_FIELD(m_spawnRadiusMax);
    GM_FIELD(m_spawnHeightMin);
    GM_FIELD(m_spawnHeightMax);
FIELDS_END()

void AttackReadyVfx::OnStart()
{
    m_player = Manager::GetObjectFirstByTag("Player");
    m_spawnInterval = Math::Max(m_spawnInterval, 0.01f);
    m_maxActiveHeads = Math::Max(m_maxActiveHeads, 1);
    m_spawnRadiusMin = Math::Max(m_spawnRadiusMin, 0.0f);
    m_spawnRadiusMax = Math::Max(m_spawnRadiusMax, m_spawnRadiusMin);
    m_spawnHeightMin = Math::Min(m_spawnHeightMin, m_spawnHeightMax);
    m_spawnHeightMax = Math::Max(m_spawnHeightMax, m_spawnHeightMin);
    m_spawnTimer = 0.0f;
}

void AttackReadyVfx::SpawnHead()
{
    if (!m_headPrefab || !m_player || !m_player->GetTransform())
    {
        return;
    }

    const Vector3 playerPosition = m_player->GetTransform()->GetPosition();
    const float angle = Random::Range(0.0f, Math::PI * 2.0f);
    const float radius = Random::Range(m_spawnRadiusMin, m_spawnRadiusMax);
    const Vector3 spawnPosition(
        playerPosition.x + std::cos(angle) * radius,
        playerPosition.y + Random::Range(m_spawnHeightMin, m_spawnHeightMax),
        playerPosition.z + std::sin(angle) * radius);

    Manager::Instantiate(
        m_headPrefab.GetPath(),
        spawnPosition,
        Vector3::Zero(),
        Vector3::One());
}

void AttackReadyVfx::Update()
{
    if (!m_player)
    {
        m_owner->Destroy();
        return;
    }

    const Player* player = m_player->GetComponent<Player>();
    if (!player || player->GetAttackState() != Player::AttackReady)
    {
        m_owner->Destroy();
        return;
    }

    m_spawnTimer -= Time::GetDeltaTime();
    if (m_spawnTimer > 0.0f)
    {
        return;
    }

    if (static_cast<int>(Manager::GetObjectsByTag("AttackChargeHead").size()) < m_maxActiveHeads)
    {
        SpawnHead();
    }

    m_spawnTimer += m_spawnInterval;
}
