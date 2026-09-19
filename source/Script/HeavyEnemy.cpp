#include "pch.h"

#include "Script/HeavyEnemy.h"

void HeavyEnemy::RegisterFields(FieldRegistry& fields)
{
    Enemy::RegisterFields(fields);
}

void HeavyEnemy::UpdateBehavior(const Vector3& playerPosition)
{
}
