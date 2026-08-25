#pragma once

#include <string>
#include <vector>

#include "System/GameObjectReference.h"
#include "System/PrefabReference.h"
#include "System/Script.h"
#include "System/Logger.h"
#include "System/YAMLDocumentIO.h"
#include "math/Matrix3x3.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"

struct ReflectNestedSettingsTest
{
private:
    int m_id = 1;
    Vector3 m_offset{ 1.0f, 2.0f, 3.0f };

    REFLECT_FRIEND(ReflectNestedSettingsTest)

public:
    ReflectNestedSettingsTest() = default;

    ReflectNestedSettingsTest(int id, const Vector3& offset)
        : m_id(id),
          m_offset(offset)
    {
    }

    bool Equals(const ReflectNestedSettingsTest& other) const
    {
        return m_id == other.m_id && m_offset == other.m_offset;
    }
};

REFLECT_BEGIN(ReflectNestedSettingsTest)
    REFLECT_FIELD(m_id),
    REFLECT_FIELD(m_offset)
REFLECT_END()

struct ReflectSettingsTest
{
private:
    int m_integer = 7;
    Vector3 m_offset{ 1.0f, 2.0f, 3.0f };
    ReflectNestedSettingsTest m_nested{ 2, { 4.0f, 5.0f, 6.0f } };
    std::vector<ReflectNestedSettingsTest> m_items{
        { 3, { 7.0f, 8.0f, 9.0f } },
        { 4, { 10.0f, 11.0f, 12.0f } },
    };

    REFLECT_FRIEND(ReflectSettingsTest)

public:
    bool Equals(const ReflectSettingsTest& other) const
    {
        if (m_integer != other.m_integer ||
            m_offset != other.m_offset ||
            !m_nested.Equals(other.m_nested) ||
            m_items.size() != other.m_items.size())
        {
            return false;
        }

        for (size_t index = 0; index < m_items.size(); ++index)
        {
            if (!m_items[index].Equals(other.m_items[index]))
            {
                return false;
            }
        }
        return true;
    }
};

REFLECT_BEGIN(ReflectSettingsTest)
    REFLECT_FIELD(m_integer),
    REFLECT_FIELD(m_offset),
    REFLECT_FIELD(m_nested),
    REFLECT_FIELD(m_items)
REFLECT_END()

// InspectorのField型とリスト編集を確認するための一時的なテスト用Script。
class FieldRegistryTest final : public Script
{
    REGISTER_COMPONENT(FieldRegistryTest, "Field Registry Test");

    bool m_boolValue = true;
    ReflectSettingsTest m_reflectSettings;
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

        YAMLValueConversionContext context;
        context.getObjectId = [this](const GameObject* object) -> std::optional<YAMLValueConversionContext::ObjectId>
        {
            return object == m_owner ? std::optional<YAMLValueConversionContext::ObjectId>(1) : std::nullopt;
        };
        context.resolveObject = [this](YAMLValueConversionContext::ObjectId id)
        {
            return id == 1 ? m_owner : nullptr;
        };

        const std::filesystem::path path = "Assets/Local/Tests/FieldRegistryTest.yaml";
        YAMLValue document;
        YAMLValue loadedDocument;
        const ReflectSettingsTest originalSettings = m_reflectSettings;
        if (fields.ToYAMLValue(document, context) &&
            YAMLDocumentIO::Save(path, document) &&
            YAMLDocumentIO::Load(path, loadedDocument) &&
            fields.FromYAMLValue(loadedDocument, context) &&
            m_reflectSettings.Equals(originalSettings))
        {
            Logger::Log("FieldRegistry Reflect field YAML round-trip test succeeded.");
        }
        else
        {
            Logger::Error("FieldRegistry Reflect field YAML round-trip test failed.");
        }

        YAMLValue reflectedValue;
        ReflectSettingsTest restoredSettings;
        const bool reflectedSuccess =
            YAMLValueConverter<ReflectSettingsTest>::ToYAMLValue(m_reflectSettings, reflectedValue, context).success &&
            YAMLValueConverter<ReflectSettingsTest>::FromYAMLValue(reflectedValue, restoredSettings, context).success &&
            restoredSettings.Equals(m_reflectSettings);
        if (reflectedSuccess)
        {
            Logger::Log("Nested/vector Reflect YAML round-trip test succeeded.");
        }
        else
        {
            Logger::Error("Nested/vector Reflect YAML round-trip test failed.");
        }

        // 解決失敗表示は往復テスト後も確認できる状態へ戻す。
        m_missingGameObjectReference.MarkResolutionError();
        m_missingPrefabReference.MarkResolutionError("Assets/Prefabs/Missing.prefab");
    }

protected:
    void RegisterFields(FieldRegistry& fields) override
    {
        GM_FIELD(m_reflectSettings);
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
