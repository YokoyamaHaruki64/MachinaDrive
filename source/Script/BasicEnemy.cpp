#include "pch.h"

#include "Script/BasicEnemy.h"

void BasicEnemy::RegisterFields(FieldRegistry& fields)
{
    Enemy::RegisterFields(fields);
}

void BasicEnemy::UpdateBehavior(const Vector3& playerPosition)
{
}
