#pragma once

#include "System/Core/Script.h"

// 複数の調整値を1つのInspector項目とYAML Mapへまとめる設定例。
struct SampleMoveSettings
{
private:
    float m_moveSpeed = 3.0f;
    float m_dashMultiplier = 2.0f;
    bool m_allowDash = true;
    Vector3 m_spawnOffset{ 0.0f, 0.0f, 1.0f };

    REFLECT_FRIEND(SampleMoveSettings)

public:
    float GetMoveSpeed() const { return m_moveSpeed; }
    float GetDashMultiplier() const { return m_dashMultiplier; }
    bool IsDashAllowed() const { return m_allowDash; }
    const Vector3& GetSpawnOffset() const { return m_spawnOffset; }
};

REFLECT_BEGIN(SampleMoveSettings)
    REFLECT_FIELD(m_moveSpeed),
    REFLECT_FIELD(m_dashMultiplier),
    REFLECT_FIELD(m_allowDash),
    REFLECT_FIELD(m_spawnOffset)
REFLECT_END()

// Script、Reflect、Input、参照、Prefab生成の基本的な流れを示すサンプル。
// GameObjectへ追加すると、WASDで移動し、Fで設定されたPrefabを生成する。
class SampleMoveScript final : public Script
{
    GM_SCRIPT(SampleMoveScript,"Sample Move Script")

    void OnStart() override;
    void Update() override;
    void LateUpdate() override;

    USE_FIELDS
    Vector3 BuildMoveDirection() const;

    SampleMoveSettings m_settings;
    GameObjectReference m_lookTarget;
    PrefabReference m_spawnPrefab{ "Assets/Prefabs/Enemy.prefab" };
    Audio* m_audio = nullptr;
};
