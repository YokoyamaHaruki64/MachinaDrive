#include "pch.h"

#if GM_ENABLE_EDITOR
#include "imgui.h"
#include "imgui_curve_custom.h"
#endif

#include "System/Editor/FieldRegistry.h"
#include "System/Core/manager.h"
#include "System/Diagnostics/Logger.h"
#include "System/Editor/FileSystem.h"

#include <array>

void FieldRegistry::ReportConversionMessage(
    const YAMLValueConversionResult& result,
    std::string_view fieldKey)
{
    if (result.message.empty())
    {
        return;
    }

    const std::string message = result.message + ": " + std::string(fieldKey);
    if (result.messageLevel == YAMLMessageLevel::Error)
    {
        Logger::Error(message);
    }
    else if (result.messageLevel == YAMLMessageLevel::Warning)
    {
        Logger::Warning(message);
    }
}

#if GM_ENABLE_EDITOR
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
#endif

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

#if GM_ENABLE_EDITOR
bool FieldRegistry::DrawValue(const FieldDescriptor& field, void* value)
{
    if (field.inspectorType == InspectorFieldType::PositionPreset && field.valueType == ValueType::Vector2)
    {
        return DrawPositionPreset(field.displayName.c_str(), *static_cast<Vector2*>(value));
    }

    switch (field.valueType)
    {
    case ValueType::Bool: return ImGui::Checkbox(field.displayName.c_str(), static_cast<bool*>(value));
    case ValueType::Int: return ImGui::DragInt(field.displayName.c_str(), static_cast<int*>(value));
    case ValueType::Float: return ImGui::DragFloat(field.displayName.c_str(), static_cast<float*>(value), 0.01f);
    case ValueType::String:
        if (field.inspectorType == InspectorFieldType::FilePath)
        {
            return DrawFilePath(field, *static_cast<std::string*>(value));
        }
        if (field.inspectorType == InspectorFieldType::Multiline)
        {
            return InputTextMultiline(field.displayName.c_str(), *static_cast<std::string*>(value));
        }
        return InputText(field.displayName.c_str(), *static_cast<std::string*>(value));
    case ValueType::Vector2: return ImGui::DragFloat2(field.displayName.c_str(), &static_cast<Vector2*>(value)->x, 0.01f);
    case ValueType::Vector3: return ImGui::DragFloat3(field.displayName.c_str(), &static_cast<Vector3*>(value)->x, 0.01f);
    case ValueType::Vector4: return ImGui::DragFloat4(field.displayName.c_str(), &static_cast<Vector4*>(value)->x, 0.01f);
    case ValueType::Color: return ImGui::ColorEdit4(field.displayName.c_str(), &static_cast<Color*>(value)->r);
    case ValueType::Matrix3x3: DrawMatrix3x3(field.displayName.c_str(), *static_cast<Matrix3x3*>(value)); return false;
    case ValueType::Matrix4x4: DrawMatrix4x4(field.displayName.c_str(), *static_cast<Matrix4x4*>(value)); return false;
    case ValueType::Quaternion: return DrawQuaternion(field.displayName.c_str(), *static_cast<Quaternion*>(value));
    case ValueType::CurveFloat: return ImGui::EditCurveFloat(field.displayName.c_str(), *static_cast<Curve<float>*>(value));
    case ValueType::CurveColor: return ImGui::EditCurveColor(field.displayName.c_str(), *static_cast<CurveColor*>(value));
    case ValueType::GameObjectReference: return DrawGameObjectReference(field.displayName.c_str(), *static_cast<GameObjectReference*>(value));
    case ValueType::PrefabReference: return DrawPrefabReference(field.displayName.c_str(), *static_cast<PrefabReference*>(value));
    default: ImGui::Text("%s: 未対応", field.displayName.c_str()); return false;
    }
}

bool FieldRegistry::InputTextMultiline(const char* label, std::string& value)
{
    if (value.capacity() < 256)
    {
        value.reserve(256);
    }

    return ImGui::InputTextMultiline(
        label,
        value.data(),
        value.capacity() + 1,
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5.0f),
        ImGuiInputTextFlags_CallbackResize,
        ResizeStringInputCallback,
        &value);
}

bool FieldRegistry::DrawPositionPreset(const char* label, Vector2& value)
{
    static constexpr const char* labels[3][3]{
        { "LT", "CT", "RT" },
        { "LC", "CC", "RC" },
        { "LB", "CB", "RB" },
    };

    ImGui::TextUnformatted(label);
    bool changed = false;
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            if (column != 0)
            {
                ImGui::SameLine();
            }

            const Vector2 preset(static_cast<float>(column) * 0.5f, static_cast<float>(row) * 0.5f);
            const bool selected = std::abs(value.x - preset.x) < 0.001f && std::abs(value.y - preset.y) < 0.001f;
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            ImGui::PushID(row * 3 + column);
            if (ImGui::Button(labels[row][column], ImVec2(34.0f, 0.0f)))
            {
                value = preset;
                changed = true;
            }
            ImGui::PopID();
            if (selected)
            {
                ImGui::PopStyleColor();
            }
        }
    }
    return changed;
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
    static constexpr std::array<std::string_view, 1> prefabExtensions{ ".prefab" };
    const bool changed = DrawPathDialog(label, path, "Assets/Prefabs", prefabExtensions);
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

bool FieldRegistry::DrawFilePath(const FieldDescriptor& field, std::string& value)
{
    std::vector<std::string_view> extensions;
    extensions.reserve(field.pathExtensions.size());
    for (const std::string& extension : field.pathExtensions)
    {
        extensions.emplace_back(extension);
    }

    return DrawPathDialog(
        field.displayName.c_str(),
        value,
        field.pathInitialDirectory,
        extensions);
}

bool FieldRegistry::DrawPathDialog(
    const char* label,
    std::string& value,
    const std::filesystem::path& initialDirectory,
    std::span<const std::string_view> extensions)
{
    ImGui::Text("%s: %s", label, value.empty() ? "None" : value.c_str());
    ImGui::SameLine();

    bool changed = false;
    if (ImGui::Button("Select"))
    {
        const std::optional<std::filesystem::path> selectedPath =
            FileSystem::OpenFileDialog(initialDirectory, extensions);
        if (selectedPath)
        {
            if (!FileSystem::IsUnderDirectory(*selectedPath, "Assets"))
            {
                Logger::Error("選択したパスはAssetsフォルダ配下に配置してください。");
            }
            else if (const std::optional<std::filesystem::path> relativePath =
                FileSystem::ToProjectRelative(*selectedPath))
            {
                value = relativePath->generic_string();
                changed = true;
            }
        }
    }

    if (!value.empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            value.clear();
            changed = true;
        }
    }

    return changed;
}

bool FieldRegistry::DrawImGui(
    InspectorFieldSpace inspectorSpace,
    bool* editStarted,
    bool* editFinished)
{
    bool changed = false;
    for (FieldDescriptor& field : m_fields)
    {
        if (field.inspectorType == InspectorFieldType::Hidden)
        {
            continue;
        }

        if (inspectorSpace != InspectorFieldSpace::All && field.inspectorSpace != inspectorSpace)
        {
            continue;
        }

        ImGui::PushID(field.key.c_str());
        if (field.drawAccessor)
        {
            changed |= field.drawAccessor(field);
        }
        else
        {
            changed |= DrawValue(field, field.value);
        }
        if (editStarted && ImGui::IsItemActivated())
        {
            *editStarted = true;
        }
        if (editFinished && ImGui::IsItemDeactivatedAfterEdit())
        {
            *editFinished = true;
        }
        ImGui::PopID();
    }
    return changed;
}
#else
bool FieldRegistry::DrawValue(const FieldDescriptor&, void*)
{
    return false;
}

bool FieldRegistry::DrawImGui(
    InspectorFieldSpace inspectorSpace,
    bool* editStarted,
    bool* editFinished)
{
    (void)inspectorSpace;
    (void)editStarted;
    (void)editFinished;
    return false;
}
#endif

bool FieldRegistry::ToYAMLValue(
    YAMLValue& output,
    const YAMLValueConversionContext& context) const
{
    try
    {
        // 登録されているFieldだけをMapへ変換し、実行時専用の値は出力しない。
        YAMLValue document = YAMLValue::Map();
        for (const FieldDescriptor& field : m_fields)
        {
            if (!field.toYAMLValue)
            {
                continue;
            }

            YAMLValue value;
            const YAMLValueConversionResult result = field.toYAMLValue(value, context);
            ReportConversionMessage(result, field.key);
            if (!result.success)
            {
                Logger::Warning("YAML保存対象外のFieldをスキップしました: " + field.key);
                continue;
            }
            document.map.emplace_back(field.key, std::move(value));
        }

        output = std::move(document);
        return true;
    }
    catch (const std::exception& exception)
    {
        Logger::Error("YAMLValueへの変換に失敗しました: " + std::string(exception.what()));
        return false;
    }
}

bool FieldRegistry::FromYAMLValue(
    const YAMLValue& input,
    const YAMLValueConversionContext& context)
{
    try
    {
        if (input.type != YAMLValue::Type::Map)
        {
            Logger::Error("YAML Field群はMapである必要があります。");
            return false;
        }

        // 変換だけ先に検証し、すべて成功した値だけを後段で反映する。
        // 全Fieldの検証完了後にまとめて反映し、途中失敗で既存状態を壊さない。
        std::vector<std::function<void()>> assignments;
        for (const FieldDescriptor& field : m_fields)
        {
            const YAMLValue* value = input.Find(field.key);
            if (!value)
            {
                continue;
            }
            if (!field.fromYAMLValue)
            {
                Logger::Error("YAML Fieldの読み込みに失敗しました: " + field.key);
                return false;
            }

            std::function<void()> assignment;
            const YAMLValueConversionResult result = field.fromYAMLValue(*value, context, assignment);
            ReportConversionMessage(result, field.key);
            if (!result.success || !assignment)
            {
                Logger::Error("YAML Fieldの読み込みに失敗しました: " + field.key);
                return false;
            }
            assignments.emplace_back(std::move(assignment));
        }

        for (const auto& [key, value] : input.map)
        {
            const auto found = std::find_if(m_fields.begin(), m_fields.end(), [&key](const FieldDescriptor& field)
            {
                return field.key == key;
            });
            if (found == m_fields.end())
            {
                Logger::Error("未登録のYAML Fieldをスキップしました: " + key);
            }
        }

        for (const std::function<void()>& assignment : assignments)
        {
            assignment();
        }
        return true;
    }
    catch (const std::exception& exception)
    {
        Logger::Error("YAMLValueからの変換に失敗しました: " + std::string(exception.what()));
        return false;
    }
}
