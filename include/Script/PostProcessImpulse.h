#pragma once

#include "Common/Easing.h"
#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"
#include "System/Serialization/YAMLReflection.h"

#include <string>
#include <unordered_set>
#include <vector>

struct ImpulseFloat
{
    std::string name;
    float value{};

    REFLECT_FRIEND(ImpulseFloat)
};

REFLECT_BEGIN(ImpulseFloat)
    REFLECT_FIELD(name),
    REFLECT_FIELD(value)
REFLECT_END()

struct ImpulseVector2
{
    std::string name;
    Vector2 value{};

    REFLECT_FRIEND(ImpulseVector2)
};

REFLECT_BEGIN(ImpulseVector2)
    REFLECT_FIELD(name),
    REFLECT_FIELD(value)
REFLECT_END()

struct ImpulseVector3
{
    std::string name;
    Vector3 value{};

    REFLECT_FRIEND(ImpulseVector3)
};

REFLECT_BEGIN(ImpulseVector3)
    REFLECT_FIELD(name),
    REFLECT_FIELD(value)
REFLECT_END()

struct ImpulseVector4
{
    std::string name;
    Vector4 value{};

    REFLECT_FRIEND(ImpulseVector4)
};

REFLECT_BEGIN(ImpulseVector4)
    REFLECT_FIELD(name),
    REFLECT_FIELD(value)
REFLECT_END()

struct ImpulseSettings
{
    std::string postProcessName;
    std::vector<ImpulseFloat> floats;
    std::vector<ImpulseVector2> vector2s;
    std::vector<ImpulseVector3> vector3s;
    std::vector<ImpulseVector4> vector4s;

    REFLECT_FRIEND(ImpulseSettings)
};

REFLECT_BEGIN(ImpulseSettings)
    REFLECT_FIELD(postProcessName),
    REFLECT_FIELD(floats),
    REFLECT_FIELD(vector2s),
    REFLECT_FIELD(vector3s),
    REFLECT_FIELD(vector4s)
REFLECT_END()

struct ImpulsePresetSettings
{
    float duration = 0.2f;
    std::string easeType{ "OutQuad" };
    std::vector<ImpulseSettings> targets;

    REFLECT_FRIEND(ImpulsePresetSettings)
};

REFLECT_BEGIN(ImpulsePresetSettings)
    REFLECT_FIELD(duration),
    REFLECT_FIELD(easeType),
    REFLECT_FIELD(targets)
REFLECT_END()

class PostProcessImpulsePreset final : public Script
{
    REGISTER_COMPONENT_OPTIONS(PostProcessImpulsePreset, "Post Process Impulse Preset", false, true);
    ImpulsePresetSettings m_settings;

public:
    PostProcessImpulsePreset() = delete;
    explicit PostProcessImpulsePreset(GameObject* owner) : Script(owner) {}
    const ImpulsePresetSettings& GetSettings() const { return m_settings; }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};

struct PostProcessImpulseEntry
{
    std::string name;
    PrefabReference prefab;

    REFLECT_FRIEND(PostProcessImpulseEntry)
};

REFLECT_BEGIN(PostProcessImpulseEntry)
    REFLECT_FIELD(name),
    REFLECT_FIELD(prefab)
REFLECT_END()

class PostProcessImpulse final : public Script
{
    REGISTER_COMPONENT_OPTIONS(PostProcessImpulse, "Post Process Impulse", false, true);

    struct ActiveImpulse
    {
        ImpulsePresetSettings settings;
        float elapsed{};
    };

    inline static PostProcessImpulse* s_instance{};
    std::vector<PostProcessImpulseEntry> m_presets;
    std::vector<ActiveImpulse> m_activeImpulses;
    std::unordered_set<std::string> m_activeTargetNames;

    bool PlayPreset(const PrefabReference& prefab);
    void ApplyImpulses();

public:
    PostProcessImpulse() = delete;
    explicit PostProcessImpulse(GameObject* owner) : Script(owner) {}
    static bool Play(std::string_view name);
    static bool Play(size_t index);
    void OnStart() override;
    void OnDestroy() override;
    void Update() override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
