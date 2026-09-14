#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Random.h"
#include "Scene/ResultScene.h"
#include "Script/Player.h"
#include "Script/PostProcessImpulse.h"
#include "Script/SceneTransition.h"
#include "Script/UI/GameHud.h"
#include "Script/WaveDirector.h"
#include "System/Editor/FieldRegistry.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char* SpawnAreaTag = "EnemySpawnArea";
    constexpr float SceneTransitionWaitDuration = 1.0f;
}

FIELDS_BEGIN(WaveDirector)
    GM_FIELD(m_basicEnemyPrefab);
    GM_FIELD(m_movingEnemyPrefab);
    GM_FIELD(m_heavyEnemyPrefab);
    GM_FIELD(m_enemySpawnVfxPrefab);
    GM_FIELD(m_timeRecoveryDropRate);
    GM_FIELD(m_maxTime);
    GM_FIELD(m_startTime);
    GM_FIELD(m_timeDrainPerSecond);
    GM_FIELD(m_baseClearTimeBonus);
    GM_FIELD(m_clearTimeGrowth);
    GM_FIELD(m_maxClearTimeBonus);
    GM_FIELD(m_waveStartDelay);
    GM_FIELD(m_waveBeginDuration);
    GM_FIELD(m_baseEnemyCount);
    GM_FIELD(m_enemyCountGrowth);
    GM_FIELD(m_enemyCountExponent);
    GM_FIELD(m_maxEnemyCount);
    GM_FIELD(m_movingStart);
    GM_FIELD(m_movingGrowth);
    GM_FIELD(m_movingMax);
    GM_FIELD(m_heavyStartWave);
    GM_FIELD(m_heavyGrowth);
    GM_FIELD(m_heavyMax);
    GM_FIELD(m_minPlayerDistance);
    GM_FIELD(m_spawnAttemptCount);
    GM_FIELD(m_spawnHeight);
FIELDS_END()

void WaveDirector::OnStart()
{
    if (GameObject* hudObject = Manager::GetFirstObjectByComponent<GameHud>())
    {
        m_gameHud = hudObject->GetComponent<GameHud>();
    }

    m_player = Manager::GetObjectFirstByTag("Player");

    m_maxTime = Math::Max(m_maxTime, 0.0f);
    m_startTime = Math::Clamp(m_startTime, 0.0f, m_maxTime);
    m_timeDrainPerSecond = Math::Max(m_timeDrainPerSecond, 0.0f);
    m_maxEnemyCount = Math::Max(m_maxEnemyCount, 1);
    m_baseEnemyCount = Math::Clamp(m_baseEnemyCount, 1, m_maxEnemyCount);
    m_enemyCountExponent = Math::Max(m_enemyCountExponent, 1.0f);
    m_spawnAttemptCount = Math::Max(m_spawnAttemptCount, 1);
    m_minPlayerDistance = Math::Max(m_minPlayerDistance, 0.0f);
    m_waveStartDelay = Math::Max(m_waveStartDelay, 0.0f);
    m_waveBeginDuration = Math::Max(m_waveBeginDuration, 0.0f);
    m_timeRecoveryDropRate = Math::Clamp(m_timeRecoveryDropRate, 0.0f, 1.0f);
    m_remainingTime = m_startTime;
    m_wave = 0;
    m_stateTimer = SceneTransitionWaitDuration;
    m_waveState = WaveState::WaitingForTransition;
    m_hasFinished = false;
    m_hasSpawnArea = ResolveSpawnArea();

    if (!m_hasSpawnArea)
    {
        Logger::Error("WaveDirector: EnemySpawnAreaタグの出現マーカーを4つ解決できません。");
        FinishGame();
        return;
    }

    // Sceneに残る旧来の手置き敵は、Wave 1の生成前に除去する。
    DestroyPlacedEnemies();
}

void WaveDirector::Update()
{
    if (m_hasFinished)
    {
        return;
    }

    switch (m_waveState)
    {
    case WaveState::WaitingForTransition:
        if (SceneTransition::IsTransitionEffect())
        {
            m_stateTimer = SceneTransitionWaitDuration;
            break;
        }

        m_stateTimer = Math::Max(0.0f, m_stateTimer - Time::GetDeltaTime());
        if (m_stateTimer <= 0.0f)
        {
            StartNextWave();
        }
        break;

    case WaveState::Begin:
        m_stateTimer = Math::Max(0.0f, m_stateTimer - Time::GetDeltaTime());
        if (m_gameHud)
        {
            const float progress = m_waveBeginDuration > 0.0f
                ? 1.0f - m_stateTimer / m_waveBeginDuration
                : 1.0f;
            m_gameHud->SetWaveBeginProgress(progress);
        }

        if (m_stateTimer <= 0.0f)
        {
            if (!SpawnWaveEnemies())
            {
                Logger::Error("WaveDirector: Waveの敵を生成できません。");
                FinishGame();
                return;
            }

            m_waveState = WaveState::Combat;
        }
        break;

    case WaveState::Combat:
        m_remainingTime = Math::Max(0.0f, m_remainingTime - m_timeDrainPerSecond * Time::GetDeltaTime());
        if (m_remainingTime <= 0.0f)
        {
            FinishGame();
            return;
        }

        if (Manager::GetObjectsByTag("Enemy").empty())
        {
            BeginWaveClear();
        }
        break;

    case WaveState::WaitingForPlayerIdle:
        if (IsPlayerIdle())
        {
            AddTime(CalculateClearTimeBonus());
            m_stateTimer = m_waveStartDelay;
            m_waveState = WaveState::Intermission;
            if (m_gameHud)
            {
                m_gameHud->SetWaveStartProgress(1.0f);
            }
        }
        break;

    case WaveState::Intermission:
        m_stateTimer -= Time::GetDeltaTime();
        if (m_stateTimer <= 0.0f)
        {
            m_stateTimer = 0.0f;
            if (m_gameHud)
            {
                m_gameHud->SetWaveStartProgress(0.0f);
            }
            StartNextWave();
        }
        else if (m_gameHud)
        {
            const float progress = m_waveStartDelay > 0.0f
                ? Math::Clamp(m_stateTimer / m_waveStartDelay, 0.0f, 1.0f)
                : 0.0f;
            m_gameHud->SetWaveStartProgress(progress);
        }
        break;
    }

    UpdateHud();
}

bool WaveDirector::ResolveSpawnArea()
{
    const std::vector<GameObject*> markers = Manager::GetObjectsByTag(SpawnAreaTag);
    if (markers.size() < 4)
    {
        return false;
    }

    GameObject* firstMarker = markers.front();
    if (!firstMarker || !firstMarker->GetTransform())
    {
        return false;
    }

    const Vector3 firstPosition = firstMarker->GetTransform()->GetPosition();
    m_minX = firstPosition.x;
    m_maxX = firstPosition.x;
    m_minZ = firstPosition.z;
    m_maxZ = firstPosition.z;
    for (GameObject* marker : markers)
    {
        if (!marker || !marker->GetTransform())
        {
            return false;
        }

        const Vector3 position = marker->GetTransform()->GetPosition();
        m_minX = Math::Min(m_minX, position.x);
        m_maxX = Math::Max(m_maxX, position.x);
        m_minZ = Math::Min(m_minZ, position.z);
        m_maxZ = Math::Max(m_maxZ, position.z);
    }

    return m_minX < m_maxX && m_minZ < m_maxZ;
}

void WaveDirector::DestroyPlacedEnemies()
{
    for (GameObject* enemy : Manager::GetObjectsByTag("Enemy"))
    {
        enemy->Destroy();
    }
}

void WaveDirector::StartNextWave()
{
    ++m_wave;
    m_stateTimer = m_waveBeginDuration;
    m_waveState = WaveState::Begin;
    if (m_gameHud)
    {
        m_gameHud->SetWaveStartProgress(0.0f);
        m_gameHud->ShowWaveBegin();
        if (m_waveBeginDuration <= 0.0f)
        {
            m_gameHud->SetWaveBeginProgress(1.0f);
        }
    }
    PostProcessImpulse::Play("WaveBegin");
    UpdateHud();
}

void WaveDirector::BeginWaveClear()
{
    m_waveState = WaveState::WaitingForPlayerIdle;
}

bool WaveDirector::IsPlayerIdle() const
{
    if (!m_player)
    {
        Logger::Error("WaveDirector: Playerが見つからないため、Wave開始を継続します。");
        return true;
    }

    const Player* player = m_player->GetComponent<Player>();
    if (!player)
    {
        Logger::Error("WaveDirector: Playerコンポーネントが見つからないため、Wave開始を継続します。");
        return true;
    }

    return player->GetAttackState() == Player::Idle && !player->IsHitStop();
}

bool WaveDirector::SpawnWaveEnemies()
{
    bool spawnedAny = false;
    const int enemyCount = CalculateEnemyCount();

    for (int i = 0; i < enemyCount; ++i)
    {
        const PrefabReference& enemyPrefab = SelectEnemyPrefab();

        if (!enemyPrefab)
        {
            Logger::Error("WaveDirector: 敵Prefabが未設定です。");
            continue;
        }

        const Vector3 spawnPosition = CreateSpawnPosition();
        if (Manager::Instantiate(
            enemyPrefab.GetPath(),
            spawnPosition,
            Vector3::Zero(),
            Vector3::One()))
        {
            spawnedAny = true;

            if (m_enemySpawnVfxPrefab)
            {
                Manager::Instantiate(
                    m_enemySpawnVfxPrefab.GetPath(),
                    spawnPosition + Vector3::Up() * 0.25f,
                    Vector3::Zero(),
                    Vector3::One());
            }
        }
    }

    return spawnedAny;
}

Vector3 WaveDirector::CreateSpawnPosition() const
{

    const Vector3 playerPosition = m_player && m_player->GetTransform()
        ? m_player->GetTransform()->GetPosition()
        : Vector3::Zero();

    Vector3 candidate;

    for (int attempt = 0; attempt < m_spawnAttemptCount; ++attempt)
    {
        candidate = Vector3(
            Random::Range(m_minX, m_maxX),
            m_spawnHeight,
            Random::Range(m_minZ, m_maxZ));

        Vector3 toPlayer = candidate - playerPosition;

        toPlayer.y = 0.0f;
        if (toPlayer.LengthSqr() >= m_minPlayerDistance * m_minPlayerDistance)
        {
            return candidate;
        }
    }

    // 狭いFieldでは、プレイヤーから最も遠い角へ出す。
    std::array<Vector3, 4> corners{
        Vector3(m_minX, m_spawnHeight, m_minZ),
        Vector3(m_minX, m_spawnHeight, m_maxZ),
        Vector3(m_maxX, m_spawnHeight, m_minZ),
        Vector3(m_maxX, m_spawnHeight, m_maxZ),
    };
    return *std::max_element(corners.begin(), corners.end(), [&playerPosition](const Vector3& left, const Vector3& right)
    {
        return (left - playerPosition).LengthSqr() < (right - playerPosition).LengthSqr();
    });
}

const PrefabReference& WaveDirector::SelectEnemyPrefab() const
{
    const float movingChance =
        Math::Clamp(m_movingStart + m_movingGrowth * static_cast<float>(m_wave - 1),
            0.0f, m_movingMax);

    const float heavyWaveOffset =
        static_cast<float>(Math::Max(0, m_wave - m_heavyStartWave));

    const float heavyChance =
        Math::Clamp(m_heavyGrowth * heavyWaveOffset, 0.0f, m_heavyMax);

    const float basicChance =
        Math::Max(0.0f, 1.0f - movingChance - heavyChance);

    const float totalChance =
        basicChance + movingChance + heavyChance;

    const float roll = Random::Range(0.0f, totalChance);

    if (roll < basicChance)
        return m_basicEnemyPrefab;

    if (roll < basicChance + movingChance)
        return m_movingEnemyPrefab;

    return m_heavyEnemyPrefab;
}

int WaveDirector::CalculateEnemyCount() const
{

    // 式: 基礎数 + (成長率 * (wave-1)^指数)

    const float waveOffset = static_cast<float>(m_wave - 1);
    const int growth = static_cast<int>(std::floor(m_enemyCountGrowth * std::pow(Math::Max(0.0f, waveOffset), m_enemyCountExponent)));
    return Math::Clamp(m_baseEnemyCount + growth, 1, m_maxEnemyCount);
}

float WaveDirector::CalculateClearTimeBonus() const
{

    // 式: 基礎ボーナス + (成長率 * sqrt(wave-1))

    const float waveOffset = static_cast<float>(m_wave - 1);
    const float bonus = m_baseClearTimeBonus + m_clearTimeGrowth * std::sqrt(Math::Max(0.0f, waveOffset));
    return Math::Clamp(bonus, 0.0f, Math::Max(0.0f, m_maxClearTimeBonus));
}

void WaveDirector::AddTime(float amount)
{
    if (amount <= 0.0f)
    {
        return;
    }

    const float previousTime = m_remainingTime;
    m_remainingTime = Math::Clamp(m_remainingTime + amount, 0.0f, m_maxTime);
    if (m_gameHud && m_remainingTime > previousTime)
    {
        m_gameHud->ShowTimeRecovery(m_remainingTime - previousTime);
    }
    UpdateHud();
}

void WaveDirector::UpdateHud() const
{
    if (!m_gameHud)
    {
        return;
    }

    m_gameHud->SetWave(m_wave);
    m_gameHud->SetEnemyCount(static_cast<int>(Manager::GetObjectsByTag("Enemy").size()));
    m_gameHud->SetHealth(m_remainingTime, m_maxTime);
}

void WaveDirector::FinishGame()
{
    if (m_hasFinished)
    {
        return;
    }

    m_hasFinished = true;
    SceneTransition::ChangeScene<ResultScene>(0.8f, EaseType::InOutQuad);
}
