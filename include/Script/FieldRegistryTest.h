#pragma once

#include <string>
#include <vector>

#include "System/GameObjectReference.h"
#include "System/PrefabReference.h"
#include "System/Script.h"
#include "math/Quaternion.h"

// InspectorのField型とリスト編集を確認するための一時的なテスト用Script。
class FieldRegistryTest final : public Script
{
    REGISTER_COMPONENT(FieldRegistryTest, "Field Registry Test");

    bool m_boolValue = true;
    int m_intValue = 42;
    float m_floatValue = 0.5f;
    std::string m_stringValue = "Inspector test";
    Quaternion m_quaternionValue = Quaternion::Identity();
    GameObjectReference m_gameObjectReference;
    GameObjectReference m_missingGameObjectReference;
    PrefabReference m_prefabReference{ "Assets/Prefabs/Test.prefab" };
    PrefabReference m_missingPrefabReference;
    std::vector<int> m_intValues{ 1, 2, 3 };
    std::vector<Vector3> m_vectorValues{ Vector3::Zero(), Vector3::One() };
    std::vector<GameObjectReference> m_gameObjectReferences = std::vector<GameObjectReference>(2);

public:
    explicit FieldRegistryTest(GameObject* owner)
        : Script(owner)
    {
        // 参照解決失敗時の表示もInspectorで確認できるようにする。
        m_missingGameObjectReference.MarkResolutionError();
        m_missingPrefabReference.MarkResolutionError("Assets/Prefabs/Missing.prefab");
    }

protected:
    void RegisterFields(FieldRegistry& fields) override
    {
        GM_FIELD(m_boolValue);
        GM_FIELD(m_intValue);
        GM_FIELD(m_floatValue);
        GM_FIELD(m_stringValue);
        GM_FIELD(m_quaternionValue);
        GM_FIELD(m_gameObjectReference);
        GM_FIELD(m_missingGameObjectReference);
        GM_FIELD(m_prefabReference);
        GM_FIELD(m_missingPrefabReference);
        GM_FIELD(m_intValues);
        GM_FIELD(m_vectorValues);
        GM_FIELD(m_gameObjectReferences);
    }
};
