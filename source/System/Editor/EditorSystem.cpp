#include "pch.h"

#include "System/Editor/EditorSystem.h"
#include "System/Core/manager.h"
#include "System/Editor/FileSystem.h"
#include "System/Editor/ImGuiStyleConfig.h"
#include "System/Core/main.h"
#include "System/Core/component.h"
#include "System/Core/GM_Time.h"
#include "System/Core/input.h"
#include "System/Diagnostics/Logger.h"
#include "System/Serialization/YAMLTypes.h"
#include "Component/Camera.h"
#include "Component/Transform.h"
#include "math/MathCommon.h"

#include "imgui.h"

#include <array>

namespace
{
    constexpr Vector3 EDITOR_CAMERA_DEFAULT_POSITION{ 0.0f, 2.0f, -5.0f };
    constexpr float EDITOR_CAMERA_LOOK_DISTANCE = 10.0f;
    constexpr float EDITOR_CAMERA_MOVE_SPEED = 10.0f;
    constexpr float EDITOR_CAMERA_FAST_MULTIPLIER = 3.0f;
    constexpr float EDITOR_CAMERA_LOOK_SENSITIVITY = 0.005f;
    constexpr float EDITOR_CAMERA_PAN_SPEED = 0.01f;
    constexpr float EDITOR_CAMERA_MAX_PITCH = Math::PI_DIV2 - 0.01f;

    Vector3 CalculateEditorCameraForward(float yaw, float pitch)
    {
        const float cosPitch = cosf(pitch);
        return Vector3(
            sinf(yaw) * cosPitch,
            sinf(pitch),
            cosf(yaw) * cosPitch).Normalized();
    }

    class LambdaEditorCommand final : public EditorCommand
    {
        std::function<void()> m_execute;
        std::function<void()> m_undo;

    public:
        LambdaEditorCommand(std::function<void()> execute, std::function<void()> undo)
            : m_execute(std::move(execute)),
            m_undo(std::move(undo))
        {
        }

        void Execute() override
        {
            m_execute();
        }

        void Undo() override
        {
            m_undo();
        }
    };

    YAMLValueConversionContext CreateEditorSnapshotContext()
    {
        YAMLValueConversionContext context;
        context.getObjectId = [](const GameObject* gameObject) -> std::optional<YAMLValueConversionContext::ObjectId>
        {
            if (!gameObject || gameObject->GetId() == 0)
            {
                return std::nullopt;
            }
            return gameObject->GetId();
        };
        context.resolveObject = [](YAMLValueConversionContext::ObjectId id) -> GameObject*
        {
            for (const std::unique_ptr<GameObject>& candidate : Manager::GetActiveGameObjects())
            {
                if (candidate->GetId() == id && !candidate->IsDestroyed())
                {
                    return candidate.get();
                }
            }
            return nullptr;
        };
        context.resolvePrefab = [](std::string_view path)
        {
            return std::filesystem::exists(std::filesystem::path(path));
        };
        return context;
    }
}

std::vector<GameObject*> EditorSystem::m_selectedGameObjects{};
GameObject* EditorSystem::m_selectionAnchor{};
GameWindow EditorSystem::m_gameWindow{};
HierarchyWindow EditorSystem::m_hierarchyWindow{};
InspectorWindow EditorSystem::m_inspectorWindow{};
PerformanceWindow EditorSystem::m_performanceWindow{};
LogWindow EditorSystem::m_logWindow{};
std::filesystem::path EditorSystem::m_pendingScenePath{};
bool EditorSystem::m_openSceneConfirmationRequested = false;
bool EditorSystem::m_applicationCloseConfirmationRequested = false;
std::vector<std::unique_ptr<EditorCommand>> EditorSystem::m_undoCommands{};
std::vector<std::unique_ptr<EditorCommand>> EditorSystem::m_redoCommands{};
std::unique_ptr<GameObject> EditorSystem::m_editorCameraObject{};
Camera* EditorSystem::m_editorCamera{};
float EditorSystem::m_editorCameraYaw = 0.0f;
float EditorSystem::m_editorCameraPitch = 0.0f;
bool EditorSystem::m_editorCameraRightDragActive = false;
bool EditorSystem::m_editorCameraMiddleDragActive = false;

void EditorSystem::Initialize()
{
    if (m_editorCamera)
    {
        return;
    }

    m_editorCameraObject = std::make_unique<GameObject>(
        "EditorCamera",
        EDITOR_CAMERA_DEFAULT_POSITION,
        Vector3::Zero(),
        Vector3::One());
    m_editorCamera = m_editorCameraObject->AddComponent<Camera>(Camera::EditorCamera);
    if (!m_editorCamera)
    {
        m_editorCameraObject.reset();
        return;
    }

    m_editorCamera->SetUp(Vector3::Up());
    UpdateEditorCameraPose();
    ActivateEditorCamera();
}

void EditorSystem::Uninitialize()
{
    Camera::SetActiveMainCamera(nullptr);
    m_editorCamera = nullptr;
    m_editorCameraObject.reset();
    m_editorCameraRightDragActive = false;
    m_editorCameraMiddleDragActive = false;
}

void EditorSystem::ActivateEditorCamera()
{
    if (m_editorCamera)
    {
        Camera::SetActiveMainCamera(m_editorCamera);
    }
}

void EditorSystem::ResetEditorCameraToScene()
{
    if (!m_editorCamera)
    {
        return;
    }

    Camera* sceneCamera = Camera::GetSceneMainCamera();
    if (!sceneCamera || !sceneCamera->transform())
    {
        m_editorCamera->transform()->SetPosition(EDITOR_CAMERA_DEFAULT_POSITION);
        m_editorCameraYaw = 0.0f;
        m_editorCameraPitch = 0.0f;
    }
    else
    {
        const Vector3 position = sceneCamera->transform()->GetPosition();
        const Vector3 forward = (sceneCamera->GetTarget() - position).Normalized();
        if (forward.LengthSqr() > 0.0f)
        {
            m_editorCameraYaw = atan2f(forward.x, forward.z);
            m_editorCameraPitch = asinf(Math::Clamp(forward.y, -1.0f, 1.0f));
        }
        else
        {
            m_editorCameraYaw = 0.0f;
            m_editorCameraPitch = 0.0f;
        }

        m_editorCamera->transform()->SetPosition(position);
    }

    UpdateEditorCameraPose();
    ActivateEditorCamera();
}

void EditorSystem::UpdateEditorCameraPose()
{
    if (!m_editorCamera || !m_editorCamera->transform())
    {
        return;
    }

    const Vector3 forward = CalculateEditorCameraForward(m_editorCameraYaw, m_editorCameraPitch);
    Transform* transform = m_editorCamera->transform();
    transform->SetEulerAngles(Vector3(m_editorCameraPitch, m_editorCameraYaw, 0.0f));
    m_editorCamera->SetTarget(transform->GetPosition() + forward * EDITOR_CAMERA_LOOK_DISTANCE);
    m_editorCamera->SetUp(Vector3::Up());
}

void EditorSystem::UpdateEditorCameraInput()
{
    if (!m_editorCamera || Manager::IsPlayMode())
    {
        return;
    }

    const bool viewportHovered = m_gameWindow.IsViewportHovered();
    const bool rightButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool middleButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (!rightButtonDown)
    {
        m_editorCameraRightDragActive = false;
    }
    else if (viewportHovered)
    {
        m_editorCameraRightDragActive = true;
    }

    if (!middleButtonDown)
    {
        m_editorCameraMiddleDragActive = false;
    }
    else if (viewportHovered && !rightButtonDown)
    {
        m_editorCameraMiddleDragActive = true;
    }

    const ImGuiIO& io = ImGui::GetIO();
    bool cameraChanged = false;

    if (m_editorCameraRightDragActive)
    {
        m_editorCameraYaw += io.MouseDelta.x * EDITOR_CAMERA_LOOK_SENSITIVITY;
        m_editorCameraPitch = Math::Clamp(
            m_editorCameraPitch - io.MouseDelta.y * EDITOR_CAMERA_LOOK_SENSITIVITY,
            -EDITOR_CAMERA_MAX_PITCH,
            EDITOR_CAMERA_MAX_PITCH);

        Vector3 movement = Vector3::Zero();
        const Vector3 forward = CalculateEditorCameraForward(m_editorCameraYaw, m_editorCameraPitch);
        const Vector3 right = Vector3::Cross(Vector3::Up(), forward).Normalized();
        if (Input::GetKeyPress(Input::KeyCode::W))
        {
            movement += forward;
        }
        if (Input::GetKeyPress(Input::KeyCode::S))
        {
            movement -= forward;
        }
        if (Input::GetKeyPress(Input::KeyCode::A))
        {
            movement -= right;
        }
        if (Input::GetKeyPress(Input::KeyCode::D))
        {
            movement += right;
        }

        if (movement.LengthSqr() > 0.0f)
        {
            const float speed = Input::GetKeyPress(Input::KeyCode::SHIFT)
                ? EDITOR_CAMERA_MOVE_SPEED * EDITOR_CAMERA_FAST_MULTIPLIER
                : EDITOR_CAMERA_MOVE_SPEED;
            m_editorCamera->transform()->Translate(
                movement.Normalized() * speed * Time::GetUnscaledDeltaTime());
            cameraChanged = true;
        }

        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
        {
            cameraChanged = true;
        }
    }

    if (m_editorCameraMiddleDragActive &&
        (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
    {
        const Vector3 forward = CalculateEditorCameraForward(m_editorCameraYaw, m_editorCameraPitch);
        const Vector3 right = Vector3::Cross(Vector3::Up(), forward).Normalized();
        const Vector3 up = Vector3::Cross(forward, right).Normalized();
        m_editorCamera->transform()->Translate(
            right * (-io.MouseDelta.x * EDITOR_CAMERA_PAN_SPEED) +
            up * (io.MouseDelta.y * EDITOR_CAMERA_PAN_SPEED));
        cameraChanged = true;
    }

    if (cameraChanged)
    {
        UpdateEditorCameraPose();
    }
}

void EditorSystem::Update()
{
    UpdateEditorCameraInput();
}

void EditorSystem::ApplyEditorCameraView()
{
    if (m_editorCamera)
    {
        m_editorCamera->SetViewProjection();
    }
}

bool EditorSystem::IsActiveGameObject(GameObject* gameObject)
{
    for (const std::unique_ptr<GameObject>& candidate : Manager::GetActiveGameObjects())
    {
        if (candidate.get() == gameObject && !candidate->IsDestroyed())
            return true;
    }
    return false;
}

bool EditorSystem::ApplyGameWindowRenderTargetSize()
{
    return m_gameWindow.ApplyRenderTargetSize();
}

bool EditorSystem::IsGameWindowReady()
{
    return m_gameWindow.IsRenderTargetReady();
}

bool EditorSystem::IsUIDisplayEnabled()
{
    return m_gameWindow.IsUIDisplayEnabled();
}

bool EditorSystem::GetGameWindowDisplayRect(Vector2& position, Vector2& size)
{
    if (!m_gameWindow.IsLayoutReady())
    {
        return false;
    }

    position = m_gameWindow.GetDisplayPosition();
    size = m_gameWindow.GetDisplaySize();
    return size.x > 0.0f && size.y > 0.0f;
}

void EditorSystem::ReloadStyle()
{
    if (!ImGuiStyleConfig::Reload())
    {
        return;
    }

    ImGuiStyleConfig::Apply();
    ImGui::GetIO().IniFilename = ImGuiStyleConfig::GetIniPathString().c_str();
}

void EditorSystem::OpenScene(const std::filesystem::path& path)
{
    if (!Manager::IsCurrentSceneDirty())
    {
        if (!Manager::LoadEditorScene(path))
        {
            Logger::Error("Sceneを読み込めません: " + path.generic_string());
        }
        return;
    }

    m_pendingScenePath = path;
    m_openSceneConfirmationRequested = true;
}

bool EditorSystem::RequestApplicationClose()
{
    if (!Manager::IsCurrentSceneDirty())
    {
        return false;
    }

    m_applicationCloseConfirmationRequested = true;
    return true;
}

bool EditorSystem::CanUndo()
{
    return !m_undoCommands.empty();
}

bool EditorSystem::CanRedo()
{
    return !m_redoCommands.empty();
}

bool EditorSystem::Undo()
{
    if (m_undoCommands.empty())
    {
        return false;
    }

    std::unique_ptr<EditorCommand> command = std::move(m_undoCommands.back());
    m_undoCommands.pop_back();
    command->Undo();
    m_redoCommands.emplace_back(std::move(command));
    Manager::MarkCurrentSceneDirty();
    return true;
}

bool EditorSystem::Redo()
{
    if (m_redoCommands.empty())
    {
        return false;
    }

    std::unique_ptr<EditorCommand> command = std::move(m_redoCommands.back());
    m_redoCommands.pop_back();
    command->Execute();
    m_undoCommands.emplace_back(std::move(command));
    Manager::MarkCurrentSceneDirty();
    return true;
}

void EditorSystem::ClearCommandHistory()
{
    m_undoCommands.clear();
    m_redoCommands.clear();
}

void EditorSystem::RecordExecutedCommand(std::unique_ptr<EditorCommand> command)
{
    if (!command)
    {
        return;
    }

    m_redoCommands.clear();
    m_undoCommands.emplace_back(std::move(command));
    Manager::MarkCurrentSceneDirty();
}

void EditorSystem::RecordExecutedCommand(std::function<void()> execute, std::function<void()> undo)
{
    if (!execute || !undo)
    {
        return;
    }

    RecordExecutedCommand(std::make_unique<LambdaEditorCommand>(std::move(execute), std::move(undo)));
}

bool EditorSystem::CaptureComponentState(Component& component, YAMLValue& output)
{
    return component.ToYAMLValue(output, CreateEditorSnapshotContext());
}

void EditorSystem::RecordComponentChange(Component& component, YAMLValue before, YAMLValue after)
{
    Component* const target = &component;
    RecordExecutedCommand(
        [target, after = std::move(after)]()
        {
            target->FromYAMLValue(after, CreateEditorSnapshotContext());
            target->OnEditorUpdate();
        },
        [target, before = std::move(before)]()
        {
            target->FromYAMLValue(before, CreateEditorSnapshotContext());
            target->OnEditorUpdate();
        });
}

void EditorSystem::DrawConfirmationDialogs()
{
    if (m_openSceneConfirmationRequested)
    {
        ImGui::OpenPopup("未保存のScene変更");
        m_openSceneConfirmationRequested = false;
    }

    if (ImGui::BeginPopupModal("未保存のScene変更", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("現在のSceneには未保存の変更があります。");
        ImGui::TextUnformatted("保存してから別のSceneを開きますか？");

        if (ImGui::Button("保存して続行"))
        {
            if (Manager::SaveCurrentScene())
            {
                OpenScene(m_pendingScenePath);
                m_pendingScenePath.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("保存せず続行"))
        {
            Manager::ClearCurrentSceneDirty();
            OpenScene(m_pendingScenePath);
            m_pendingScenePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル"))
        {
            m_pendingScenePath.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_applicationCloseConfirmationRequested)
    {
        ImGui::OpenPopup("終了前の確認");
        m_applicationCloseConfirmationRequested = false;
    }

    if (ImGui::BeginPopupModal("終了前の確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("現在のSceneには未保存の変更があります。");
        ImGui::TextUnformatted("保存して終了しますか？");

        if (ImGui::Button("保存して終了"))
        {
            if (Manager::SaveCurrentScene())
            {
                ImGui::CloseCurrentPopup();
                DestroyWindow(GetWindow());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("保存せず終了"))
        {
            Manager::ClearCurrentSceneDirty();
            ImGui::CloseCurrentPopup();
            DestroyWindow(GetWindow());
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void EditorSystem::DrawToolbar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))
    {
        Undo();
    }
    if (!io.WantTextInput && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))
    {
        Redo();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, CanUndo()))
        {
            Undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, CanRedo()))
        {
            Redo();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::MenuItem("Style Reload"))
        {
            ReloadStyle();
        }
        ImGui::EndMenu();
    }

    if (!Manager::IsPlayMode())
    {
        if (ImGui::MenuItem("Open Scene"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::OpenFileDialog("Assets/Scenes", ".scene");
            const std::optional<std::filesystem::path> projectPath = selectedPath
                ? FileSystem::ToProjectRelative(*selectedPath)
                : std::nullopt;
            if (projectPath)
            {
                OpenScene(*projectPath);
            }
        }
    }

    ImGui::EndMainMenuBar();
}

void EditorSystem::DrawImGui()
{
    DrawToolbar();
    DrawConfirmationDialogs();

    std::erase_if(m_selectedGameObjects, [](GameObject* gameObject)
    {
        return !IsActiveGameObject(gameObject);
    });
    if (!IsActiveGameObject(m_selectionAnchor))
    {
        m_selectionAnchor = m_selectedGameObjects.empty() ? nullptr : m_selectedGameObjects.back();
    }

    m_hierarchyWindow.SetSelectedGameObjects(m_selectedGameObjects, m_selectionAnchor);
    m_inspectorWindow.SetSelectedGameObjects(m_selectedGameObjects);

    const std::array<EditorWindow*, 5> windows = {
        &m_gameWindow,
        &m_hierarchyWindow,
        &m_inspectorWindow,
        &m_performanceWindow,
        &m_logWindow,
    };
    for (EditorWindow* window : windows)
    {
        window->DrawImGui();
    }
}
