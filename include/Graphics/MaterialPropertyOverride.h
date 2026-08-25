#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "Graphics/GraphicInc.h"

class MaterialPropertyOverride final
{
    struct PropertyNameHash
    {
        using is_transparent = void;

        size_t operator()(std::string_view name) const
        {
            return std::hash<std::string_view>{}(name);
        }
    };

    using PropertyMap = std::unordered_map<std::string, PropertyValue, PropertyNameHash, std::equal_to<>>;
    PropertyMap m_values;

public:
    void Set(std::string_view name, PropertyValue value)
    {
        const auto found = m_values.find(name);
        if (found != m_values.end())
        {
            found->second = std::move(value);
            return;
        }
        m_values.emplace(std::string(name), std::move(value));
    }

    void SetFloat(std::string_view name, float value) { Set(name, value); }
    void SetInt(std::string_view name, int value) { Set(name, value); }
    void SetBool(std::string_view name, bool value) { Set(name, value); }
    void SetVector2(std::string_view name, const Vector2& value) { Set(name, value); }
    void SetVector3(std::string_view name, const Vector3& value) { Set(name, value); }
    void SetVector4(std::string_view name, const Vector4& value) { Set(name, value); }
    void SetMatrix3x3(std::string_view name, const Matrix3x3& value) { Set(name, value); }
    void SetMatrix4x4(std::string_view name, const Matrix4x4& value) { Set(name, value); }
    void SetColor(std::string_view name, const Color& value) { Set(name, value); }
    void SetTexture(std::string_view name, TextureHandle value) { Set(name, value); }

    const PropertyValue* Find(std::string_view name) const
    {
        const auto found = m_values.find(name);
        return found == m_values.end() ? nullptr : &found->second;
    }

    bool Empty() const { return m_values.empty(); }
    void Clear() { m_values.clear(); }
    void Clear(std::string_view name)
    {
        const auto found = m_values.find(name);
        if (found != m_values.end())
        {
            m_values.erase(found);
        }
    }

    const PropertyMap& GetValues() const { return m_values; }
};
