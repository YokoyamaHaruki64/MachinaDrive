#include "pch.h"

#include "imgui.h"

#include "System/FieldRegistry.h"
#include "System/Manager.h"

int FieldRegistry::ResizeStringInputCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::string* value = static_cast<std::string*>(data->UserData);
        value->resize(data->BufTextLen);
        data->Buf = value->data();
    }
    return 0;
}

bool FieldRegistry::InputText(const char* label, std::string& value)
{
    if (value.capacity() == 0)
    {
        value.reserve(32);
    }

    return ImGui::InputText(label, value.data(), value.capacity() + 1,
        ImGuiInputTextFlags_CallbackResize, ResizeStringInputCallback, &value);
}

void FieldRegistry::DrawMatrix3x3(const char* label, Matrix3x3& value)
{
    ImGui::TextUnformatted(label);
    ImGui::Indent();
    for (int row = 0; row < 3; ++row)
    {
        ImGui::PushID(row);
        ImGui::DragFloat3("##Row", value.m[row], 0.01f);
        ImGui::PopID();
    }
    ImGui::Unindent();
}

void FieldRegistry::DrawMatrix4x4(const char* label, Matrix4x4& value)
{
    ImGui::TextUnformatted(label);
    ImGui::Indent();
    for (int row = 0; row < 4; ++row)
    {
        ImGui::PushID(row);
        ImGui::DragFloat4("##Row", value.m[row], 0.01f);
        ImGui::PopID();
    }
    ImGui::Unindent();
}

std::string FieldRegistry::CreateDisplayName(std::string_view key)
{
    if (key.starts_with("m_"))
    {
        key.remove_prefix(2);
    }

    std::string result;
    for (char character : key)
    {
        if (character == '_')
        {
            result += ' ';
            continue;
        }

        if (!result.empty() && isupper(static_cast<unsigned char>(character)))
        {
            result += ' ';
        }
        result += result.empty() ? static_cast<char>(toupper(static_cast<unsigned char>(character))) : character;
    }
    return result;
}

bool FieldRegistry::DrawValue(const FieldDescriptor& field, void* value)
{
    switch (field.valueType)
    {
    case ValueType::Bool: return ImGui::Checkbox(field.displayName.c_str(), static_cast<bool*>(value));
    case ValueType::Int: return ImGui::DragInt(field.displayName.c_str(), static_cast<int*>(value));
    case ValueType::Float: return ImGui::DragFloat(field.displayName.c_str(), static_cast<float*>(value), 0.01f);
    case ValueType::String: return InputText(field.displayName.c_str(), *static_cast<std::string*>(value));
    case ValueType::Vector2: return ImGui::DragFloat2(field.displayName.c_str(), &static_cast<Vector2*>(value)->x, 0.01f);
    case ValueType::Vector3: return ImGui::DragFloat3(field.displayName.c_str(), &static_cast<Vector3*>(value)->x, 0.01f);
    case ValueType::Vector4: return ImGui::DragFloat4(field.displayName.c_str(), &static_cast<Vector4*>(value)->x, 0.01f);
    case ValueType::Color: return ImGui::ColorEdit4(field.displayName.c_str(), &static_cast<Color*>(value)->r);
    case ValueType::Matrix3x3: DrawMatrix3x3(field.displayName.c_str(), *static_cast<Matrix3x3*>(value)); return false;
    case ValueType::Matrix4x4: DrawMatrix4x4(field.displayName.c_str(), *static_cast<Matrix4x4*>(value)); return false;
    case ValueType::Quaternion: return DrawQuaternion(field.displayName.c_str(), *static_cast<Quaternion*>(value));
    case ValueType::GameObjectReference: return DrawGameObjectReference(field.displayName.c_str(), *static_cast<GameObjectReference*>(value));
    case ValueType::PrefabReference: return DrawPrefabReference(field.displayName.c_str(), *static_cast<PrefabReference*>(value));
    default: ImGui::Text("%s: 未対応", field.displayName.c_str()); return false;
    }
}

bool FieldRegistry::DrawQuaternion(const char* label, Quaternion& value)
{
    if (!ImGui::DragFloat4(label, &value.x, 0.01f))
    {
        return false;
    }

    value = value.Normalized();
    return true;
}

bool FieldRegistry::DrawGameObjectReference(const char* label, GameObjectReference& value)
{
    const char* preview = value.IsResolutionError() ? "Error" : (value ? value->GetName().c_str() : "None");
    bool changed = false;

    if (ImGui::BeginCombo(label, preview))
    {
        if (ImGui::Selectable("None", !value && !value.IsResolutionError()))
        {
            value.Clear();
            changed = true;
        }

        for (const std::unique_ptr<GameObject>& gameObject : Manager::GetActiveGameObjects())
        {
            if (gameObject->IsDestroyed())
            {
                continue;
            }

            if (ImGui::Selectable(gameObject->GetName().c_str(), value.Get() == gameObject.get()))
            {
                value.Set(gameObject.get());
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    if (value.IsResolutionError())
    {
        ImGui::TextDisabled("参照解決に失敗しました。");
    }
    return changed;
}

bool FieldRegistry::DrawPrefabReference(const char* label, PrefabReference& value)
{
    std::string path = value.GetPath();
    const bool changed = InputText(label, path);
    if (changed)
    {
        value.SetPath(std::move(path));
    }

    if (value.IsResolutionError())
    {
        ImGui::TextDisabled("Error: %s", value.GetPath().c_str());
    }
    else if (!value)
    {
        ImGui::TextDisabled("None");
    }
    return changed;
}

void FieldRegistry::DrawImGui(InspectorFieldSpace inspectorSpace)
{
    for (FieldDescriptor& field : m_fields)
    {
        if (inspectorSpace != InspectorFieldSpace::All && field.inspectorSpace != inspectorSpace)
        {
            continue;
        }

        ImGui::PushID(field.key.c_str());
        if (field.drawAccessor)
        {
            field.drawAccessor(field);
        }
        else
        {
            DrawValue(field, field.value);
        }
        ImGui::PopID();
    }
}
