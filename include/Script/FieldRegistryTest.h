#pragma once

#include <string>
#include <vector>

#include "System/GameObjectReference.h"
#include "System/PrefabReference.h"
#include "System/Script.h"
#include "System/Logger.h"
#include "System/YAMLArchive.h"
#include "math/Matrix3x3.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"

// InspectorのField型とリスト編集を確認するための一時的なテスト用Script。
class FieldRegistryTest final : public Script
{
    REGISTER_COMPONENT(FieldRegistryTest, "Field Registry Test");

    bool m_boolValue = true;
    int m_intValue = 42;
    float m_floatValue = 0.5f;
    std::string m_stringValue = "Inspector test";
    Matrix3x3 m_matrix3x3Value = Matrix3x3::Identity();
    Matrix4x4 m_matrix4x4Value = Matrix4x4::Identity();
    Quaternion m_quaternionValue = Quaternion::Identity();
    GameObjectReference m_gameObjectReference;
    GameObjectReference m_missingGameObjectReference;
    PrefabReference m_prefabReference{ "Assets/Prefabs/Test.prefab" };
    PrefabReference m_missingPrefabReference;
    std::vector<int> m_intValues{ 1, 2, 3 };
    std::vector<Vector3> m_vectorValues{ Vector3::Zero(), Vector3::One() };
    std::vector<Quaternion> m_quaternionValues{ Quaternion::Identity() };

public:
    explicit FieldRegistryTest(GameObject* owner)
        : Script(owner)
    {
        // 参照解決失敗時の表示もInspectorで確認できるようにする。
        m_gameObjectReference.Set(owner);
        m_missingGameObjectReference.MarkResolutionError();
        m_missingPrefabReference.MarkResolutionError("Assets/Prefabs/Missing.prefab");
    }

    void OnStart() override
    {
        FieldRegistry fields;
        RegisterFields(fields);

        YAMLArchiveContext context;
        context.getObjectId = [this](const GameObject* object) -> std::optional<YAMLArchiveContext::ObjectId>
        {
            return object == m_owner ? std::optional<YAMLArchiveContext::ObjectId>(1) : std::nullopt;
        };
        context.resolveObject = [this](YAMLArchiveContext::ObjectId id)
        {
            return id == 1 ? m_owner : nullptr;
        };

        const std::filesystem::path path = "Assets/Local/Tests/FieldRegistryTest.yaml";
        if (YAMLArchive::SaveFields(path, fields, context) && YAMLArchive::LoadFields(path, fields, context))
        {
            Logger::Log("FieldRegistry YAML round-trip test succeeded.");
        }

        // 解決失敗表示は往復テスト後も確認できる状態へ戻す。
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
        GM_FIELD(m_matrix3x3Value);
        GM_FIELD(m_matrix4x4Value);
        GM_FIELD(m_quaternionValue);
        GM_FIELD(m_gameObjectReference);
        GM_FIELD(m_missingGameObjectReference);
        GM_FIELD(m_prefabReference);
        GM_FIELD(m_missingPrefabReference);
        GM_FIELD(m_intValues);
        GM_FIELD(m_vectorValues);
        GM_FIELD(m_quaternionValues);
    }
};
