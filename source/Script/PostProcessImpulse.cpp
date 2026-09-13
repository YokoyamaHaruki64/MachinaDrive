#include "pch.h"

#include "Script/PostProcessImpulse.h"

#include "Component/PostEffect.h"
#include "Graphics/MaterialPropertyOverride.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Core/GM_Time.h"
#include "System/Core/manager.h"
#include "System/Rendering/renderer.h"

#include <algorithm>
#include <type_traits>

namespace
{
    void AddFloat(MaterialPropertyOverride& properties, const std::string& name, float value)
    {
        const PropertyValue* current = properties.Find(name);
        const float total = current && std::holds_alternative<float>(*current)
            ? std::get<float>(*current) + value
            : value;
        properties.SetFloat(name, total);
    }

    void AddVector2(MaterialPropertyOverride& properties, const std::string& name, const Vector2& value)
    {
        const PropertyValue* current = properties.Find(name);
        const Vector2 total = current && std::holds_alternative<Vector2>(*current)
            ? std::get<Vector2>(*current) + value
            : value;
        properties.SetVector2(name, total);
    }

    void AddVector3(MaterialPropertyOverride& properties, const std::string& name, const Vector3& value)
    {
        const PropertyValue* current = properties.Find(name);
        const Vector3 total = current && std::holds_alternative<Vector3>(*current)
            ? std::get<Vector3>(*current) + value
            : value;
        properties.SetVector3(name, total);
    }

    void AddVector4(MaterialPropertyOverride& properties, const std::string& name, const Vector4& value)
    {
        const PropertyValue* current = properties.Find(name);
        const Vector4 total = current && std::holds_alternative<Vector4>(*current)
            ? std::get<Vector4>(*current) + value
            : value;
        properties.SetVector4(name, total);
    }

    void SetFinalProperties(
        PostProcess& postProcess,
        const MaterialPropertyOverride& impulseProperties)
    {
        Material* material = postProcess.GetMaterial();
        MaterialPropertyOverride& finalProperties = postProcess.GetImpulseOverride();
        finalProperties.Clear();

        if (!material)
            return;

        for (const auto& [name, impulseValue] : impulseProperties.GetValues())
        {
            const ShaderPropertyDesc* property = material->GetShader().GetProperty(name);
            if (!property)
                continue;

            PropertyValue baseValue;
            if (!material->GetPropertyValue(*property, baseValue))
                continue;

            std::visit(
                [&finalProperties, &name, &impulseValue](const auto& typedBaseValue)
                {
                    using ValueType = std::decay_t<decltype(typedBaseValue)>;

                    if constexpr (
                        std::is_same_v<ValueType, float> ||
                        std::is_same_v<ValueType, Vector2> ||
                        std::is_same_v<ValueType, Vector3> ||
                        std::is_same_v<ValueType, Vector4>)
                    {
                        if (const ValueType* typedImpulseValue = std::get_if<ValueType>(&impulseValue))
                            finalProperties.Set(name, typedBaseValue + *typedImpulseValue);
                    }
                },
                baseValue);
        }
    }
}

void PostProcessImpulsePreset::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_settings);
}

void PostProcessImpulse::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_presets);
}

void PostProcessImpulse::OnStart()
{
    s_instance = this;
}

void PostProcessImpulse::OnDestroy()
{
    if (s_instance == this)
        s_instance = nullptr;
}

bool PostProcessImpulse::Play(std::string_view name)
{
    if (!s_instance)
        return false;

    const auto iterator = std::find_if(
        s_instance->m_presets.begin(),
        s_instance->m_presets.end(),
        [name](const PostProcessImpulseEntry& entry)
        {
            return entry.name == name;
        });

    return iterator != s_instance->m_presets.end()
        && s_instance->PlayPreset(iterator->prefab);
}

bool PostProcessImpulse::Play(size_t index)
{
    return s_instance
        && index < s_instance->m_presets.size()
        && s_instance->PlayPreset(s_instance->m_presets[index].prefab);
}

bool PostProcessImpulse::PlayPreset(const PrefabReference& prefab)
{
    if (!prefab)
        return false;

    GameObject* presetObject = Manager::Instantiate(prefab.GetPath());
    PostProcessImpulsePreset* preset = presetObject
        ? presetObject->GetComponent<PostProcessImpulsePreset>()
        : nullptr;
    if (!preset)
    {
        if (presetObject)
            presetObject->Destroy();
        return false;
    }

    m_activeImpulses.push_back({ preset->GetSettings(), 0.0f });
    presetObject->Destroy();
    return true;
}

void PostProcessImpulse::Update()
{
    ApplyImpulses();

    const float deltaTime = Time::GetDeltaTime();
    for (ActiveImpulse& impulse : m_activeImpulses)
        impulse.elapsed += deltaTime;

    std::erase_if(m_activeImpulses, [](const ActiveImpulse& impulse)
        {
            return impulse.elapsed >= impulse.settings.duration;
        });
}

void PostProcessImpulse::ApplyImpulses()
{
    std::unordered_map<std::string, MaterialPropertyOverride> overrides;

    for (const std::string& name : m_activeTargetNames)
    {
        if (PostProcess* postProcess = Renderer::FindPostEffect(name))
            postProcess->GetImpulseOverride().Clear();
    }
    m_activeTargetNames.clear();

    for (const ActiveImpulse& impulse : m_activeImpulses)
    {
        const float duration = std::max(impulse.settings.duration, 0.0001f);
        const float progress = 1.0f - impulse.elapsed / duration;
        const float strength = Easing::Evaluate(
            progress,
            Easing::ToEaseType(impulse.settings.easeType));

        for (const ImpulseSettings& target : impulse.settings.targets)
        {
            m_activeTargetNames.insert(target.postProcessName);
            MaterialPropertyOverride& properties = overrides[target.postProcessName];
            for (const ImpulseFloat& property : target.floats)
                AddFloat(properties, property.name, property.value * strength);
            for (const ImpulseVector2& property : target.vector2s)
                AddVector2(properties, property.name, property.value * strength);
            for (const ImpulseVector3& property : target.vector3s)
                AddVector3(properties, property.name, property.value * strength);
            for (const ImpulseVector4& property : target.vector4s)
                AddVector4(properties, property.name, property.value * strength);
        }
    }

    for (const auto& [name, properties] : overrides)
    {
        PostProcess* postProcess = Renderer::FindPostEffect(name);
        if (postProcess)
            SetFinalProperties(*postProcess, properties);
    }
}
