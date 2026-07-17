#include "pch.h"
#include "System/Manager.h"

#include "Script/Enemy.h"
#include "Script/Score.h"

void Enemy::Update()
{
}

void Enemy::OnDestroy()
{
    auto obj = Manager::GetFirstObjectByComponent<Score>();
    if (obj)
    {
        Score* scoreComponent = obj->GetComponent<Score>();
        if (scoreComponent)
        {
            scoreComponent->AddScore(75);
        }
    }
}
