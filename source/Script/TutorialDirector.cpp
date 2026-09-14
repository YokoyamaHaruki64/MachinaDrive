#include "pch.h"

#include "Component/ComponentInc.h"

#include "Component/Rigidbody.h"
#include "Component/Transform.h"
#include "Component/UIText.h"
#include "Scene/GameScene.h"
#include "Script/Player.h"
#include "Script/TutorialDirector.h"
#include "Script/SceneTransition.h"
#include "Script/PostProcessImpulse.h"
#include "System/Core/GameObject.h"
#include "System/Core/GM_Time.h"
#include "System/Diagnostics/Logger.h"
#include "System/Core/manager.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(TutorialDirector)
    GM_FIELD(m_player);
    GM_FIELD(m_instructionText);
    GM_FIELD(m_basicEnemyPrefab);
    GM_FIELD(m_enemySpawnPosition);
    GM_FIELD(m_completeWaitDuration);
FIELDS_END()

void TutorialDirector::OnStart()
{
    if (!m_player)
    {
        m_player = Manager::GetObjectFirstByTag("Player");
    }

    if (m_player)
    {
        m_playerScript = m_player->GetComponent<Player>();
        m_playerRigidbody = m_player->GetComponent<Rigidbody>();
        if (m_player->GetTransform())
        {
            m_centerPosition = m_player->GetTransform()->GetPosition();
        }
    }

    if (m_instructionText)
    {
        m_instructionTextComponent = m_instructionText->GetComponent<UIText>();
    }

    m_completeWaitDuration = Math::Max(m_completeWaitDuration, 0.0f);
    m_stateTimer = 0.0f;
    m_state = TutorialState::WaitingForShot;

    SetInstruction("左クリックしながら動かして離す");
}

void TutorialDirector::Update()
{
    if (!m_playerScript || !m_playerRigidbody)
    {
        return;
    }

    switch (m_state)
    {
    case TutorialState::WaitingForShot:
        if ((m_playerScript->GetAttackState() == Player::Attacking
                || m_playerScript->GetAttackState() == Player::AttackEnd)
            && m_playerScript->GetShotDirection().LengthSqr() > 0.01f)
        {
            m_state = TutorialState::WaitingForPlayerStop;
            SetInstruction("止まるまで次の発射は出来ない");
        }
        break;

    case TutorialState::WaitingForPlayerStop:
        if (m_playerScript->GetAttackState() == Player::Idle
            && !m_playerScript->IsHitStop())
        {
            ReturnToCenter();
        }
        break;

    case TutorialState::WaitingForEnemyDefeat:
        if (!m_enemy)
        {
            CompleteTutorial();
        }
        break;

    case TutorialState::Complete:
        m_stateTimer = Math::Max(0.0f, m_stateTimer - Time::GetDeltaTime());
        if (m_stateTimer <= 0.0f)
        {
            SceneTransition::ChangeScene<GameScene>(0.8f, EaseType::InOutQuad);
        }
        break;
    }
}

void TutorialDirector::SetInstruction(const char* text)
{
    if (m_instructionTextComponent)
    {
        m_instructionTextComponent->SetText(text ? text : "");
    }
}

void TutorialDirector::ReturnToCenter()
{
    Transform* playerTransform = m_player ? m_player->GetTransform() : nullptr;
    if (!playerTransform)
    {
        return;
    }

    // 帰還は演出付きの瞬間移動とし、プレイヤーの操作状態を引き継ぐ。
    playerTransform->SetPosition(m_centerPosition);
    m_playerRigidbody->SetVelocity(Vector3::Zero());
    PostProcessImpulse::Play("WaveBegin");
    SpawnBasicEnemy();
}

void TutorialDirector::SpawnBasicEnemy()
{
    m_state = TutorialState::WaitingForEnemyDefeat;
    SetInstruction("ぶつかって敵を倒せ！");

    if (!m_basicEnemyPrefab)
    {
        Logger::Error("TutorialDirector: BasicEnemyのPrefabが設定されていません。");
        CompleteTutorial();
        return;
    }

    m_enemy = Manager::Instantiate(
        m_basicEnemyPrefab.GetPath(),
        m_enemySpawnPosition,
        Vector3::Zero(),
        Vector3::One());
    if (!m_enemy)
    {
        Logger::Error("TutorialDirector: BasicEnemyを生成できません。");
        CompleteTutorial();
    }
}

void TutorialDirector::CompleteTutorial()
{
    m_state = TutorialState::Complete;
    m_stateTimer = m_completeWaitDuration;
    SetInstruction("Complete!!");
}
