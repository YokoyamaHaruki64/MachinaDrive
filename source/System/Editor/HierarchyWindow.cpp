#include "pch.h"

#include <optional>

#include "imgui.h"

#include "Component/UIButton.h"
#include "Component/UICanvas.h"
#include "Component/UIImage.h"
#include "Component/TransformBase.h"
#include "Component/UIText.h"
#include "Component/Transform.h"
#include "System/Editor/FileSystem.h"
#include "System/Editor/EditorSystem.h"
#include "System/Editor/HierarchyWindow.h"
#include "System/Diagnostics/Logger.h"
#include "System/Core/manager.h"

std::optional<std::filesystem::path> HierarchyWindow::ToAssetRelativePath(
    const std::filesystem::path& selectedPath)
{
    if (_wcsicmp(selectedPath.extension().c_str(), L".prefab") != 0)
    {
        Logger::Error("Prefabファイルの拡張子は.prefabにしてください。");
        return std::nullopt;
    }

    if (!FileSystem::IsUnderDirectory(selectedPath, "Assets"))
    {
        Logger::Error("PrefabはAssetsフォルダ配下に配置してください。");
        return std::nullopt;
    }

    return FileSystem::ToProjectRelative(selectedPath);
}

void HierarchyWindow::DrawGameObjectNode(GameObject& gameObject)
{
    if (gameObject.IsDestroyed())
    {
        return;
    }

    const TransformBase* const transform = gameObject.GetTransformBase();
    const std::vector<TransformBase*>& children = transform->GetChildren();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (std::find(m_selectedGameObjects->begin(), m_selectedGameObjects->end(), &gameObject) != m_selectedGameObjects->end())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(&gameObject);
    const bool isOpen = ImGui::TreeNodeEx(gameObject.GetName().c_str(), flags);
    m_visibleGameObjects.push_back(&gameObject);
    if (ImGui::IsItemClicked())
    {
        const ImGuiIO& io = ImGui::GetIO();
        m_selectionRequest = SelectionRequest{ &gameObject, io.KeyCtrl, io.KeyShift };
    }

    if (ImGui::BeginDragDropSource())
    {
        // クリック選択を確定する前にドラッグが始まった場合は、選択変更を取り消す。
        m_selectionRequest.reset();
        m_draggedGameObjects.clear();
        if (std::find(m_selectedGameObjects->begin(), m_selectedGameObjects->end(), &gameObject)
            != m_selectedGameObjects->end())
        {
            for (GameObject* selectedGameObject : *m_selectedGameObjects)
            {
                if (selectedGameObject && !selectedGameObject->IsDestroyed())
                {
                    m_draggedGameObjects.push_back(selectedGameObject);
                }
            }
        }
        if (m_draggedGameObjects.empty())
        {
            m_draggedGameObjects.push_back(&gameObject);
        }

        std::vector<GameObject*>* draggedGameObjects = &m_draggedGameObjects;
        ImGui::SetDragDropPayload("HierarchyGameObject", &draggedGameObjects, sizeof(draggedGameObjects));
        ImGui::TextUnformatted(gameObject.GetName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyGameObject"))
        {
            std::vector<GameObject*>* const draggedGameObjects =
                *static_cast<std::vector<GameObject*>* const*>(payload->Data);
            if (draggedGameObjects)
            {
                ApplyParentToDraggedGameObjects(*draggedGameObjects, &gameObject);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem("GameObjectContext"))
    {
        if (ImGui::MenuItem("Create Prefab"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::SaveFileDialog("Assets", ".prefab", "New.prefab");
            if (selectedPath)
            {
                if (const std::optional<std::filesystem::path> assetPath = ToAssetRelativePath(*selectedPath))
                {
                    if (Manager::SavePrefab(gameObject, *assetPath))
                    {
                        Manager::MarkCurrentSceneDirty();
                        EditorSystem::ClearCommandHistory();
                    }
                }
            }
        }
        if (ImGui::MenuItem("Delete"))
        {
            if (std::find(m_selectedGameObjects->begin(), m_selectedGameObjects->end(), &gameObject)
                == m_selectedGameObjects->end())
            {
                m_selectedGameObjects->assign(1, &gameObject);
                *m_selectionAnchor = &gameObject;
            }
            DeleteSelectedGameObjects();
        }
        ImGui::EndPopup();
    }

    if (isOpen)
    {
        for (TransformBase* child : children)
        {
            if (child && child->GetOwner() && !child->GetOwner()->IsDestroyed())
            {
                DrawGameObjectNode(*child->GetOwner());
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void HierarchyWindow::DrawRootDropTarget()
{
    ImGui::Separator();
    ImGui::TextUnformatted("Root (drop here)");
    ImGui::InvisibleButton("RootDropTarget", ImVec2(-FLT_MIN, 28.0f));

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyGameObject"))
        {
            std::vector<GameObject*>* const draggedGameObjects =
                *static_cast<std::vector<GameObject*>* const*>(payload->Data);
            if (draggedGameObjects)
            {
                ApplyParentToDraggedGameObjects(*draggedGameObjects, nullptr);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyWindow::DrawImGui()
{
    if (!m_selectedGameObjects || !m_selectionAnchor)
    {
        return;
    }

    BeginWindow("Hierarchy", Vector2(260.0f, 400.0f));

    if (ImGui::Button("Save Scene"))
    {
        Manager::SaveCurrentScene();
    }
    ImGui::SameLine();

    const std::filesystem::path scenePath = Manager::GetCurrentSceneFilePath();
    const std::string sceneName = scenePath.empty() ? "(None)" : scenePath.filename().string();
    ImGui::Text("Scene: %s", sceneName.c_str());
    ImGui::Separator();

    m_visibleGameObjects.clear();
    for (const std::unique_ptr<GameObject>& gameObject : Manager::GetActiveGameObjects())
    {
        if (!gameObject->GetParent())
        {
            DrawGameObjectNode(*gameObject);
        }
    }

    DrawRootDropTarget();
    ApplySelectionRequest();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
        && !ImGui::GetIO().WantTextInput
        && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        DeleteSelectedGameObjects();
    }

    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            GameObject* const createdGameObject = Manager::CreateEmpty();
            m_selectedGameObjects->assign(1, createdGameObject);
            *m_selectionAnchor = createdGameObject;
            EditorSystem::RecordExecutedCommand(
                Manager::CreateEditorCreationCommand({ createdGameObject }));
        }
        if (ImGui::BeginMenu("UI"))
        {
            if (ImGui::MenuItem("Create Canvas"))
            {
                GameObject* const createdCanvas = Manager::CreateUICanvas();
                m_selectedGameObjects->assign(1, createdCanvas);
                *m_selectionAnchor = createdCanvas;
                EditorSystem::RecordExecutedCommand(
                    Manager::CreateEditorCreationCommand({ createdCanvas }));
            }
            if (ImGui::MenuItem("Create Text"))
            {
                CreateUIObject("Text", true, false, false);
            }
            if (ImGui::MenuItem("Create Image"))
            {
                CreateUIObject("Image", false, true, false);
            }
            if (ImGui::MenuItem("Create Button"))
            {
                CreateUIObject("Button", false, true, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Instantiate Prefab"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::OpenFileDialog("Assets", ".prefab");
            if (selectedPath)
            {
                if (const std::optional<std::filesystem::path> assetPath = ToAssetRelativePath(*selectedPath))
                {
                    GameObject* const instantiatedGameObject = Manager::Instantiate(*assetPath);
                    if (instantiatedGameObject)
                    {
                        m_selectedGameObjects->assign(1, instantiatedGameObject);
                        *m_selectionAnchor = instantiatedGameObject;
                        EditorSystem::RecordExecutedCommand(
                            Manager::CreateEditorCreationCommand({ instantiatedGameObject }));
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    EndWindow();
}

void HierarchyWindow::ApplySelectionRequest()
{
    if (!m_selectionRequest)
    {
        return;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_selectionRequest.reset();
        return;
    }

    // IsItemClickedは押下時に通知されるため、解放されるまで選択を保留する。
    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        return;
    }

    const SelectionRequest request = *m_selectionRequest;
    m_selectionRequest.reset();
    if (request.range && *m_selectionAnchor)
    {
        const auto anchor = std::find(m_visibleGameObjects.begin(), m_visibleGameObjects.end(), *m_selectionAnchor);
        const auto target = std::find(m_visibleGameObjects.begin(), m_visibleGameObjects.end(), request.gameObject);
        if (anchor != m_visibleGameObjects.end() && target != m_visibleGameObjects.end())
        {
            const auto first = std::min(anchor, target);
            const auto last = std::max(anchor, target);
            m_selectedGameObjects->assign(first, std::next(last));
            return;
        }
    }

    if (request.toggle)
    {
        const auto selected = std::find(m_selectedGameObjects->begin(), m_selectedGameObjects->end(), request.gameObject);
        if (selected == m_selectedGameObjects->end())
        {
            m_selectedGameObjects->push_back(request.gameObject);
            *m_selectionAnchor = request.gameObject;
        }
        else
        {
            m_selectedGameObjects->erase(selected);
            if (*m_selectionAnchor == request.gameObject)
            {
                *m_selectionAnchor = m_selectedGameObjects->empty() ? nullptr : m_selectedGameObjects->back();
            }
        }
        return;
    }

    m_selectedGameObjects->assign(1, request.gameObject);
    *m_selectionAnchor = request.gameObject;
}

void HierarchyWindow::DeleteSelectedGameObjects()
{
    if (!m_selectedGameObjects || m_selectedGameObjects->empty())
    {
        return;
    }

    const std::vector<GameObject*> selectedGameObjects = *m_selectedGameObjects;
    std::unique_ptr<EditorCommand> command = Manager::CreateEditorDeletionCommand(selectedGameObjects);
    if (!command)
    {
        return;
    }

    m_selectedGameObjects->clear();
    *m_selectionAnchor = nullptr;
    EditorSystem::RecordExecutedCommand(std::move(command));
}

void HierarchyWindow::CreateUIObject(std::string_view name, bool addText, bool addImage, bool addButton)
{
    GameObject* canvas = Manager::GetFirstObjectByComponent<UICanvas>();
    const bool createdCanvas = canvas == nullptr;
    if (createdCanvas)
    {
        canvas = Manager::CreateUICanvas();
    }

    GameObject* const gameObject = Manager::CreateUIObject(std::string(name));
    gameObject->SetParent(canvas, ParentTransformMode::KeepLocal);
    if (addText)
    {
        gameObject->AddComponent<UIText>();
    }
    if (addImage)
    {
        gameObject->AddComponent<UIImage>();
    }
    if (addButton)
    {
        gameObject->AddComponent<UIButton>();
    }

    m_selectedGameObjects->assign(1, gameObject);
    *m_selectionAnchor = gameObject;

    std::vector<GameObject*> createdObjects;
    if (createdCanvas)
    {
        createdObjects.push_back(canvas);
    }
    createdObjects.push_back(gameObject);
    EditorSystem::RecordExecutedCommand(Manager::CreateEditorCreationCommand(createdObjects));
}

void HierarchyWindow::ApplyParentToDraggedGameObjects(
    const std::vector<GameObject*>& draggedGameObjects,
    GameObject* newParent)
{
    struct ParentChange
    {
        GameObject* gameObject = nullptr;
        GameObject* previousParent = nullptr;
    };

    std::vector<ParentChange> changes;
    changes.reserve(draggedGameObjects.size());
    for (GameObject* gameObject : draggedGameObjects)
    {
        if (!gameObject || gameObject->IsDestroyed() || gameObject == newParent)
        {
            continue;
        }

        GameObject* const previousParent = gameObject->GetParent();
        if (previousParent == newParent)
        {
            continue;
        }

        if (gameObject->SetParent(newParent))
        {
            changes.push_back({ gameObject, previousParent });
        }
    }

    if (changes.empty())
    {
        return;
    }

    const std::vector<ParentChange> undoChanges = changes;
    EditorSystem::RecordExecutedCommand(
        [changes = std::move(changes), newParent]()
        {
            for (const ParentChange& change : changes)
            {
                if (!change.gameObject->IsDestroyed())
                {
                    change.gameObject->SetParent(newParent);
                }
            }
        },
        [undoChanges]()
        {
            for (auto iterator = undoChanges.rbegin(); iterator != undoChanges.rend(); ++iterator)
            {
                if (!iterator->gameObject->IsDestroyed())
                {
                    iterator->gameObject->SetParent(iterator->previousParent);
                }
            }
        });
}
