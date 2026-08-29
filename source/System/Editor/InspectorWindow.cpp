#include "pch.h"

#include "imgui.h"

#include "Component/Transform.h"
#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Editor/FileSystem.h"
#include "System/Editor/EditorSystem.h"
#include "System/Serialization/YAMLTypes.h"
#include "System/Editor/InspectorWindow.h"
#include "System/Diagnostics/Logger.h"
#include "System/Core/manager.h"
#include "System/Rendering/RendererBase.h"
#include <algorithm>
#include <typeinfo>

std::optional<std::filesystem::path> InspectorWindow::ToAssetRelativePath(
    const std::filesystem::path& selectedPath,
    const std::filesystem::path& assetDirectory,
    const wchar_t* extension,
    const char* assetName)
{
    if (_wcsicmp(selectedPath.extension().c_str(), extension) != 0)
    {
        Logger::Error(std::string(assetName) + "ファイルの拡張子が不正です。");
        return std::nullopt;
    }

    if (!FileSystem::IsUnderDirectory(selectedPath, assetDirectory))
    {
        Logger::Error(std::string(assetName) + "は" + assetDirectory.generic_string() +
            "フォルダ配下に配置してください。");
        return std::nullopt;
    }

    return FileSystem::ToProjectRelative(selectedPath);
}

void InspectorWindow::CreateMaterial(RendererBase& renderer, uint32_t slot)
{
    const std::optional<std::filesystem::path> selectedPath =
        FileSystem::OpenFileDialog("Assets/Shaders", ".shader");
    if (!selectedPath)
    {
        return;
    }

    const std::optional<std::filesystem::path> shaderPath = ToAssetRelativePath(
        *selectedPath,
        "Assets",
        L".shader",
        "Shader");
    if (!shaderPath)
    {
        return;
    }

    GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderPath->generic_string());
    if (!shader)
    {
        return;
    }

    Material* material = MaterialManager::Create(shaderPath->stem().string() + " Material", *shader);
    if (material)
    {
        renderer.SetMaterial(slot, material);
    }
}

void InspectorWindow::SelectMaterial(RendererBase& renderer, uint32_t slot)
{
    const std::optional<std::filesystem::path> selectedPath =
        FileSystem::OpenFileDialog("Assets/Materials", ".material");
    if (!selectedPath)
    {
        return;
    }

    const std::optional<std::filesystem::path> materialPath = ToAssetRelativePath(
        *selectedPath,
        "Assets",
        L".material",
        "Material");
    if (!materialPath)
    {
        return;
    }

    Material* material = MaterialManager::Load(*materialPath);
    if (material && material->IsValid())
    {
        renderer.SetMaterial(slot, material);
    }
}

void InspectorWindow::SelectShader(Material& material)
{
    const std::optional<std::filesystem::path> selectedPath =
        FileSystem::OpenFileDialog("Assets/Shaders", ".shader");
    if (!selectedPath)
    {
        return;
    }

    const std::optional<std::filesystem::path> shaderPath = ToAssetRelativePath(
        *selectedPath,
        "Assets",
        L".shader",
        "Shader");
    if (!shaderPath)
    {
        return;
    }

    GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderPath->generic_string());
    if (!shader || !material.SetShader(*shader))
    {
        Logger::Error("MaterialのShaderを設定できませんでした。\nShader: " + shaderPath->generic_string());
    }
}

void InspectorWindow::DrawMaterialName(Material& material)
{
    auto [bufferIterator, inserted] = m_materialNameBuffers.try_emplace(&material);
    std::array<char, 256>& buffer = bufferIterator->second;
    if (inserted)
    {
        const std::string& name = material.GetName();
        const size_t length = std::min(name.size(), buffer.size() - 1);
        std::copy_n(name.data(), length, buffer.data());
        buffer[length] = '\0';
    }

    const bool submitted = ImGui::InputText(
        "Name",
        buffer.data(),
        buffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    if (!submitted && !ImGui::IsItemDeactivatedAfterEdit())
    {
        return;
    }

    if (!MaterialManager::Rename(material, buffer.data()))
    {
        const std::string& name = material.GetName();
        const size_t length = std::min(name.size(), buffer.size() - 1);
        std::fill(buffer.begin(), buffer.end(), '\0');
        std::copy_n(name.data(), length, buffer.data());
    }
}

void InspectorWindow::DrawMaterialInspector(RendererBase& renderer, uint32_t slot)
{
    ImGui::PushID(static_cast<int>(slot));

    Material* material = renderer.GetMaterial(slot);
    const std::string materialPathBefore = material ? material->GetFilePath() : std::string{};
    std::string label = "Material Slot " + std::to_string(slot);
    if (!material)
    {
        label += " (None)";
    }
    else if (!material->IsValid())
    {
        label += " (Error)";
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode(label.c_str()))
    {
        if (ImGui::Button("Select Material"))
        {
            SelectMaterial(renderer, slot);
        }

        if (!material || !material->IsValid())
        {
            ImGui::TextUnformatted(material ? "Status: Error" : "Status: None");
            if (ImGui::Button("Create Material"))
            {
                CreateMaterial(renderer, slot);
            }
        }
        else
        {
            DrawMaterialName(*material);
            ImGui::Text("Shader: %s", material->GetShader().GetFilePath().c_str());
            if (ImGui::Button("Select Shader"))
            {
                SelectShader(*material);
            }
            material->DrawInspectorImGui();

            const std::string& materialPath = material->GetFilePath();
            ImGui::Text("Path: %s", materialPath.empty() ? "Unsaved" : materialPath.c_str());
            if (materialPath.empty())
            {
                if (ImGui::Button("Save Material As"))
                {
                    const std::string defaultFileName = material->GetName() + ".material";
                    const std::optional<std::filesystem::path> selectedPath =
                        FileSystem::SaveFileDialog("Assets/Materials", ".material", defaultFileName);
                    if (selectedPath)
                    {
                        const std::optional<std::filesystem::path> assetPath = ToAssetRelativePath(
                            *selectedPath,
                            "Assets",
                            L".material",
                            "Material");
                        if (assetPath)
                        {
                            MaterialManager::Save(*material, *assetPath);
                        }
                    }
                }
            }
            else if (ImGui::Button("Save Material"))
            {
                MaterialManager::Save(*material, materialPath);
            }
        }

        ImGui::TreePop();
    }

    Material* const materialAfter = renderer.GetMaterial(slot);
    if (materialAfter != material || (materialAfter && materialAfter->GetFilePath() != materialPathBefore))
    {
        // Materialの割り当てと未保存Materialの保存先はScene側にも保持する。
        Manager::MarkCurrentSceneDirty();
        EditorSystem::ClearCommandHistory();
    }

    ImGui::PopID();
}

void InspectorWindow::DrawImGui()
{
    BeginWindow("Inspector", Vector2(360.0f, 500.0f));

    if (!m_selectedGameObjects || m_selectedGameObjects->empty())
    {
        m_gameObjectNameTarget = nullptr;
        m_componentEditSnapshots.clear();
        ImGui::TextUnformatted("GameObjectを選択してください。");
        EndWindow();
        return;
    }

    if (m_selectedGameObjects->size() > 1)
    {
        m_gameObjectNameTarget = nullptr;
        m_componentEditSnapshots.clear();
        ImGui::Text("%zu 個のGameObjectを選択中です。", m_selectedGameObjects->size());
        EndWindow();
        return;
    }

    GameObject* selectedGameObject = m_selectedGameObjects->front();
    if (m_gameObjectNameTarget != selectedGameObject)
    {
        m_gameObjectNameTarget = selectedGameObject;
        m_componentEditSnapshots.clear();
        std::fill(m_gameObjectNameBuffer.begin(), m_gameObjectNameBuffer.end(), '\0');
        const std::string& name = selectedGameObject->GetName();
        const size_t length = std::min(name.size(), m_gameObjectNameBuffer.size() - 1);
        std::copy_n(name.data(), length, m_gameObjectNameBuffer.data());
    }

    const bool nameSubmitted = ImGui::InputText(
        "Name",
        m_gameObjectNameBuffer.data(),
        m_gameObjectNameBuffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    if (nameSubmitted || ImGui::IsItemDeactivatedAfterEdit())
    {
        const std::string name = m_gameObjectNameBuffer.data();
        if (name != selectedGameObject->GetName())
        {
            const std::string previousName = selectedGameObject->GetName();
            selectedGameObject->SetName(name);
            EditorSystem::RecordExecutedCommand(
                [gameObject = selectedGameObject, name]() { gameObject->SetName(name); },
                [gameObject = selectedGameObject, previousName]() { gameObject->SetName(previousName); });
        }
    }
    bool active = selectedGameObject->IsActive();
    if (ImGui::Checkbox("Active", &active))
    {
        const bool previousActive = selectedGameObject->IsActive();
        selectedGameObject->SetActive(active);
        EditorSystem::RecordExecutedCommand(
            [gameObject = selectedGameObject, active]() { gameObject->SetActive(active); },
            [gameObject = selectedGameObject, previousActive]() { gameObject->SetActive(previousActive); });
    }
    ImGui::Text("Started: %s", selectedGameObject->IsStarted() ? "Yes" : "No");
    ImGui::Text("Destroy Requested: %s", selectedGameObject->IsDestroyed() ? "Yes" : "No");
    ImGui::Separator();

    if (!selectedGameObject->GetPrefabPath().empty())
    {
        ImGui::Text("Prefab: %s",
            selectedGameObject->IsPrefabResolutionError()
                ? "Error"
                : selectedGameObject->GetPrefabPath().c_str());
        if (ImGui::Button("Apply to Prefab"))
        {
            Manager::ApplyToPrefab(*selectedGameObject);
        }
        ImGui::Separator();
    }

    const std::vector<Component*> components = selectedGameObject->GetComponents();
    for (size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex)
    {
        Component* component = components[componentIndex];
        ImGui::PushID(static_cast<int>(componentIndex));
        const ComponentRegistration* registration = ComponentRegistry::Find(typeid(*component));
        const char* componentName = registration ? registration->displayName.c_str() : typeid(*component).name();
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        const bool componentOpen = ImGui::TreeNode(componentName);
        ImGui::SameLine();
        bool enabled = component->IsEnabled();
        if (ImGui::Checkbox("Enabled", &enabled))
        {
            const bool previousEnabled = component->IsEnabled();
            component->SetEnabled(enabled);
            EditorSystem::RecordExecutedCommand(
                [component, enabled]() { component->SetEnabled(enabled); },
                [component, previousEnabled]() { component->SetEnabled(previousEnabled); });
        }

        if (componentOpen)
        {
            YAMLValue before;
            const bool canUndo = EditorSystem::CaptureComponentState(*component, before);
            bool editStarted = false;
            bool editFinished = false;
            InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All;
            if (dynamic_cast<Transform*>(component))
            {
                ImGui::Checkbox("Local", &m_showLocalTransform);
                inspectorSpace = m_showLocalTransform ? InspectorFieldSpace::Local : InspectorFieldSpace::World;
            }

            if (component->DrawInspectorImGui(inspectorSpace, &editStarted, &editFinished))
            {
                component->OnEditorUpdate();
                Manager::MarkCurrentSceneDirty();
            }

            if (editStarted && canUndo && !m_componentEditSnapshots.contains(component))
            {
                m_componentEditSnapshots.emplace(component, std::move(before));
            }

            if (editFinished)
            {
                YAMLValue after;
                const auto snapshot = m_componentEditSnapshots.find(component);
                if (snapshot != m_componentEditSnapshots.end() && EditorSystem::CaptureComponentState(*component, after))
                {
                    EditorSystem::RecordComponentChange(*component, std::move(snapshot->second), std::move(after));
                    m_componentEditSnapshots.erase(snapshot);
                }
            }

            if (RendererBase* renderer = dynamic_cast<RendererBase*>(component))
            {
                for (uint32_t slot = 0; slot < renderer->GetMaterialSlotCount(); ++slot)
                {
                    DrawMaterialInspector(*renderer, slot);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        for (ComponentCategory category : { ComponentCategory::Component, ComponentCategory::Script })
        {
            const char* categoryName = category == ComponentCategory::Component ? "Component" : "Script";
            if (ImGui::BeginMenu(categoryName))
            {
                for (const ComponentRegistration& registration : ComponentRegistry::GetRegistrations())
                {
                    if (!registration.showInAddComponent || registration.category != category)
                    {
                        continue;
                    }

                    const bool canAdd = ComponentRegistry::CanAdd(registration, *selectedGameObject);
                    ImGui::BeginDisabled(!canAdd);
                    if (ImGui::MenuItem(registration.displayName.c_str()))
                    {
                        ComponentRegistry::Add(registration, *selectedGameObject);
                        Manager::MarkCurrentSceneDirty();
                        EditorSystem::ClearCommandHistory();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled();
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }

    EndWindow();
}
