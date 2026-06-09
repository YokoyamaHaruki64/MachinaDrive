#include "pch.h"

#include "Script/Bullet.h"
#include "Component/Transform.h"

void Bullet::Update()
{
    transform()->SetPosition(transform()->GetPosition() + m_velocity * 0.016f);
}
