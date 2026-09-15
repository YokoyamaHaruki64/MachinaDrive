#pragma once

#include "System/Core/Script.h"

// Playerの引っ張り方向を表示する一時的なガイド。
class ShotDirector final : public Script
{
    GM_SCRIPT(ShotDirector, "Shot Director")
    USE_FIELDS

public:
    void OnStart() override;
    void Update() override;

    void SetPlayer(GameObject* player);

private:
    GameObjectReference m_player;

    // ガイドの中心を、プレイヤーから方向へどれだけずらすか。
    float m_pivotOffset = 2.5f;
    Vector3 m_positionOffset{ 0.0f, 0.05f, 0.0f };

    // Prefabで調整した平面の向きを維持したまま、Y軸だけを更新する。
    Vector3 m_baseRotation{ Vector3::Zero() };
    Vector3 m_lastDirection{ Vector3::Forward() };

    Vector3 BuildShotWorldDirection(
        const Vector2& shotDirection,
        const Vector3& playerPosition);

};
