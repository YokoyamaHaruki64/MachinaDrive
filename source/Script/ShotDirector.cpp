#include "pch.h"

#include "Component/ComponentInc.h"

#include "Component/Camera.h"
#include "Script/Player.h"
#include "Script/ShotDirector.h"
#include "math/Vector2.h"

#include <cmath>

FIELDS_BEGIN(ShotDirector)
    GM_FIELD(m_pivotOffset);
    GM_FIELD(m_positionOffset);
FIELDS_END()

void ShotDirector::OnStart()
{
    if (!m_player)
    {
        GameObject* playerObject = Manager::GetFirstObjectByComponent<Player>();
        if (playerObject)
        {
            m_player = playerObject;
        }
    }

    m_baseRotation = transform()->GetEulerAngles();
}

void ShotDirector::Update()
{
    if (!m_player)
    {
        return;
    }

    Player* player = m_player->GetComponent<Player>();
    Transform* playerTransform = m_player->GetTransform();
    if (!player || !playerTransform)
    {
        return;
    }

    const Vector3 playerPosition = playerTransform->GetPosition();
    const Vector2 shotDirection = player->GetShotDirection();
    if (shotDirection.LengthSqr() <= 0.0f)
    {
        transform()->SetPosition(playerPosition + m_lastDirection * m_pivotOffset + m_positionOffset);
        return;
    }

    m_lastDirection = BuildShotWorldDirection(shotDirection, playerPosition);
    if (m_lastDirection.LengthSqr() <= 0.0f)
    {
        return;
    }

    const float yaw = std::atan2(m_lastDirection.x, m_lastDirection.z);
    transform()->SetRotation(Vector3(
        m_baseRotation.x,
        m_baseRotation.y + yaw,
        m_baseRotation.z));
    transform()->SetPosition(playerPosition + m_lastDirection * m_pivotOffset + m_positionOffset);
}

void ShotDirector::SetPlayer(GameObject* player)
{
    m_player = player;
}

Vector3 ShotDirector::BuildShotWorldDirection(const Vector2& shotDirection, const Vector3& playerPosition)
{
    Camera* camera = Camera::GetMainCamera();
    if (camera)
    {
        Vector3 cameraForward = playerPosition - camera->transform()->GetPosition();
        cameraForward.y = 0.0f;
        if (cameraForward.LengthSqr() > 0.0f)
        {
            cameraForward.Normalize();

            Vector3 cameraRight = Vector3::Cross(Vector3::Up(), cameraForward);
            cameraRight.y = 0.0f;
            cameraRight.Normalize();

            const Vector3 worldDirection =
                cameraForward * shotDirection.y + cameraRight * -shotDirection.x;
            if (worldDirection.LengthSqr() > 0.0f)
            {
                return worldDirection.Normalized();
            }
        }
    }

    return Vector3(-shotDirection.x, 0.0f, shotDirection.y).Normalized();

}
