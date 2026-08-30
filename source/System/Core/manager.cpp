#include "pch.h"

#include "System/Core/manager.h"

#include <algorithm>
#include <unordered_set>

#if GM_ENABLE_EDITOR
#include "System/Editor/EditorSystem.h"
#include "System/Editor/ImGuiStyleConfig.h"
#endif
#include "System/Serialization/GameObjectYamlSerializer.h"
#include "System/Core/input.h"
#include "System/Diagnostics/Logger.h"
#include "System/Diagnostics/PerformanceMonitor.h"
#include "System/Rendering/renderer.h"
#include "System/Rendering/RendererBase.h"
#include "Graphics/TextureManager.h"
#include "System/UI/FontAsset.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Scene/Scene.h"
#include "Scene/TitleScene.h"

#include "Component/Camera.h"
#include "Component/PostEffect.h"
#include "Component/Transform.h"
#include "Component/TransformBase.h"
#include "Component/audio.h"

#if GM_ENABLE_EDITOR
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_curve_custom.h"
#endif

std::vector<std::unique_ptr<GameObject>> Manager::m_GameObjects;
std::vector<std::unique_ptr<GameObject>> Manager::m_pendingGameObjects; // オブジェクト追加用の一時的な格納場所
std::vector<std::unique_ptr<PostProcess>> Manager::m_postEffects;         // ポストエフェクトのリスト

std::unique_ptr<Scene> Manager::m_currentScene;
std::unique_ptr<Scene> Manager::m_nextScene;
Manager::SceneTransitionPhase Manager::m_sceneTransitionPhase = Manager::SceneTransitionPhase::None;
Manager::GameMode Manager::m_gameMode =
#if GM_ENABLE_EDITOR
    Manager::GameMode::Edit;
#else
    Manager::GameMode::Play;
#endif
bool Manager::m_stopPlayRequested = false;
bool Manager::m_sceneDirty = false;
uint64_t Manager::m_nextGameObjectId = 1;

std::vector<RendererBase*> Manager::m_Renderers; // 描画用のレンダラーリスト

uint64_t Manager::AllocateGameObjectId()
{
    auto isUsed = [](uint64_t id)
    {
        for (const auto& gameObject : m_GameObjects)
        {
            if (gameObject->GetId() == id)
            {
                return true;
            }
        }
        for (const auto& gameObject : m_pendingGameObjects)
        {
            if (gameObject->GetId() == id)
            {
                return true;
            }
        }
        return false;
    };

    if (m_nextGameObjectId == 0)
    {
        m_nextGameObjectId = 1;
    }

    while (isUsed(m_nextGameObjectId))
    {
        ++m_nextGameObjectId;
        if (m_nextGameObjectId == 0)
        {
            m_nextGameObjectId = 1;
        }
    }

    const uint64_t allocatedId = m_nextGameObjectId;
    ++m_nextGameObjectId;
    if (m_nextGameObjectId == 0)
    {
        m_nextGameObjectId = 1;
    }
    return allocatedId;
}

void Manager::EnsureGameObjectIds(
    std::vector<std::unique_ptr<GameObject>>& gameObjects,
    bool assignNewIds)
{
    if (assignNewIds)
    {
        for (const auto& gameObject : gameObjects)
        {
            gameObject->SetId(0);
        }
    }

    for (const auto& gameObject : gameObjects)
    {
        if (gameObject->GetId() == 0)
        {
            gameObject->SetId(AllocateGameObjectId());
            continue;
        }

        if (gameObject->GetId() >= m_nextGameObjectId)
        {
            m_nextGameObjectId = gameObject->GetId() + 1;
            if (m_nextGameObjectId == 0)
            {
                m_nextGameObjectId = 1;
            }
        }
    }
}

void Manager::DestroyGameObjects()
{
    // まだ開始していない旧シーンの生成物は、そのまま所有権を破棄する。
    m_pendingGameObjects.clear();

    for (auto& gameObject : m_GameObjects)
    {
        gameObject->OnDestroy();
    }
    m_GameObjects.clear();

    // OnDestroyから生成されたオブジェクトを次のシーンへ持ち越さない。
    m_pendingGameObjects.clear();
}

#if GM_ENABLE_EDITOR
Manager::EditorDetachedGameObjects Manager::DetachForEditor(const std::vector<GameObject*>& gameObjects)
{
    EditorDetachedGameObjects detachedGameObjects;
    if (m_gameMode != GameMode::Edit)
    {
        return detachedGameObjects;
    }

    std::unordered_set<GameObject*> selectedGameObjects;
    selectedGameObjects.reserve(gameObjects.size());
    for (GameObject* gameObject : gameObjects)
    {
        if (gameObject && !gameObject->IsDestroyed())
        {
            selectedGameObjects.emplace(gameObject);
        }
    }

    if (selectedGameObjects.empty())
    {
        return detachedGameObjects;
    }

    std::unordered_set<GameObject*> detachedGameObjectSet;
    detachedGameObjectSet.reserve(m_GameObjects.size());
    for (const std::unique_ptr<GameObject>& gameObject : m_GameObjects)
    {
        bool shouldDetach = selectedGameObjects.contains(gameObject.get());
        for (TransformBase* parent = gameObject->GetTransformBase()->GetParent(); !shouldDetach && parent; parent = parent->GetParent())
        {
            shouldDetach = selectedGameObjects.contains(parent->GetOwner());
        }
        if (shouldDetach)
        {
            detachedGameObjectSet.emplace(gameObject.get());
        }
    }

    std::vector<std::unique_ptr<GameObject>> remainingGameObjects;
    remainingGameObjects.reserve(m_GameObjects.size());
    for (std::unique_ptr<GameObject>& gameObject : m_GameObjects)
    {
        if (detachedGameObjectSet.contains(gameObject.get()))
        {
            TransformBase* const previousParentTransform = gameObject->GetTransformBase()->GetParent();
            GameObject* const previousParent = previousParentTransform ? previousParentTransform->GetOwner() : nullptr;
            if (previousParent && !detachedGameObjectSet.contains(previousParent))
            {
                gameObject->SetParent(nullptr, ParentTransformMode::KeepLocal);
            }

            for (Component* component : gameObject->GetComponents())
            {
                if (RendererBase* renderer = dynamic_cast<RendererBase*>(component))
                {
                    UnregisterRenderer(renderer);
                }
            }
            detachedGameObjects.entries.push_back({ std::move(gameObject), previousParent });
        }
        else
        {
            remainingGameObjects.emplace_back(std::move(gameObject));
        }
    }
    m_GameObjects = std::move(remainingGameObjects);

    return detachedGameObjects;
}

void Manager::RestoreForEditor(EditorDetachedGameObjects& detachedGameObjects)
{
    if (m_gameMode != GameMode::Edit || detachedGameObjects.Empty())
    {
        return;
    }

    std::vector<std::pair<GameObject*, GameObject*>> restoredGameObjects;
    restoredGameObjects.reserve(detachedGameObjects.entries.size());
    for (EditorDetachedGameObjects::Entry& entry : detachedGameObjects.entries)
    {
        GameObject* const restoredGameObject = entry.gameObject.get();
        m_GameObjects.emplace_back(std::move(entry.gameObject));
        restoredGameObjects.emplace_back(restoredGameObject, entry.previousParent);
    }

    // HierarchyのRoot表示をUndo前後で安定させるため、復元時だけ永続ID順へ戻す。
    std::sort(m_GameObjects.begin(), m_GameObjects.end(),
        [](const std::unique_ptr<GameObject>& left, const std::unique_ptr<GameObject>& right)
        {
            return left->GetId() < right->GetId();
        });

    for (const auto& [gameObject, previousParent] : restoredGameObjects)
    {
        if (previousParent)
        {
            gameObject->SetParent(previousParent, ParentTransformMode::KeepLocal);
        }

        for (Component* component : gameObject->GetComponents())
        {
            if (RendererBase* renderer = dynamic_cast<RendererBase*>(component))
            {
                RegisterRenderer(renderer);
            }
        }
    }
    detachedGameObjects.entries.clear();
}

void Manager::DestroyDetachedForEditor(EditorDetachedGameObjects& detachedGameObjects)
{
    for (EditorDetachedGameObjects::Entry& entry : detachedGameObjects.entries)
    {
        if (entry.gameObject)
        {
            entry.gameObject->OnDestroy();
        }
    }
    detachedGameObjects.entries.clear();
}

std::unique_ptr<Component> Manager::DetachComponentForEditor(GameObject& gameObject, Component& component)
{
    if (m_gameMode != GameMode::Edit || &component == gameObject.GetTransformBase())
    {
        return nullptr;
    }

    const auto iterator = std::find_if(
        gameObject.m_components.begin(),
        gameObject.m_components.end(),
        [&component](const std::unique_ptr<Component>& candidate)
        {
            return candidate.get() == &component;
        });
    if (iterator == gameObject.m_components.end())
    {
        return nullptr;
    }

    if (RendererBase* renderer = dynamic_cast<RendererBase*>(&component))
    {
        UnregisterRenderer(renderer);
    }

    std::unique_ptr<Component> detachedComponent = std::move(*iterator);
    gameObject.m_components.erase(iterator);
    return detachedComponent;
}

void Manager::RestoreComponentForEditor(GameObject& gameObject, std::unique_ptr<Component>& component)
{
    if (m_gameMode != GameMode::Edit || !component)
    {
        return;
    }

    Component* const restoredComponent = component.get();
    gameObject.m_components.emplace_back(std::move(component));
    if (RendererBase* renderer = dynamic_cast<RendererBase*>(restoredComponent))
    {
        RegisterRenderer(renderer);
    }
}

void Manager::DestroyDetachedComponentForEditor(std::unique_ptr<Component>& component)
{
    if (component)
    {
        component->OnDestroy();
        component.reset();
    }
}

class Manager::EditorObjectLifetimeCommand final : public EditorCommand
{
    bool m_isCreation;
    std::vector<GameObject*> m_gameObjects;
    EditorDetachedGameObjects m_detachedGameObjects;

public:
    EditorObjectLifetimeCommand(
        bool isCreation,
        std::vector<GameObject*> gameObjects,
        EditorDetachedGameObjects detachedGameObjects = {})
        : m_isCreation(isCreation)
        , m_gameObjects(std::move(gameObjects))
        , m_detachedGameObjects(std::move(detachedGameObjects))
    {
    }

    ~EditorObjectLifetimeCommand() override
    {
        Manager::DestroyDetachedForEditor(m_detachedGameObjects);
    }

    void Execute() override
    {
        if (m_isCreation)
        {
            Manager::RestoreForEditor(m_detachedGameObjects);
            return;
        }

        m_detachedGameObjects = Manager::DetachForEditor(m_gameObjects);
    }

    void Undo() override
    {
        if (m_isCreation)
        {
            m_detachedGameObjects = Manager::DetachForEditor(m_gameObjects);
            return;
        }

        Manager::RestoreForEditor(m_detachedGameObjects);
    }
};

class Manager::EditorComponentDeletionCommand final : public EditorCommand
{
    GameObject* m_gameObject = nullptr;
    Component* m_component = nullptr;
    std::unique_ptr<Component> m_detachedComponent;

public:
    EditorComponentDeletionCommand(
        GameObject& gameObject,
        Component& component,
        std::unique_ptr<Component> detachedComponent)
        : m_gameObject(&gameObject)
        , m_component(&component)
        , m_detachedComponent(std::move(detachedComponent))
    {
    }

    ~EditorComponentDeletionCommand() override
    {
        Manager::DestroyDetachedComponentForEditor(m_detachedComponent);
    }

    void Execute() override
    {
        if (m_gameObject && m_component)
        {
            m_detachedComponent = Manager::DetachComponentForEditor(*m_gameObject, *m_component);
        }
    }

    void Undo() override
    {
        if (m_gameObject)
        {
            Manager::RestoreComponentForEditor(*m_gameObject, m_detachedComponent);
        }
    }
};

std::unique_ptr<EditorCommand> Manager::CreateEditorCreationCommand(
    const std::vector<GameObject*>& gameObjects)
{
    if (gameObjects.empty())
    {
        return nullptr;
    }

    return std::make_unique<EditorObjectLifetimeCommand>(true, gameObjects);
}

std::unique_ptr<EditorCommand> Manager::CreateEditorDeletionCommand(
    const std::vector<GameObject*>& selectedGameObjects)
{
    EditorDetachedGameObjects detachedGameObjects = DetachForEditor(selectedGameObjects);
    if (detachedGameObjects.Empty())
    {
        return nullptr;
    }

    return std::make_unique<EditorObjectLifetimeCommand>(
        false,
        selectedGameObjects,
        std::move(detachedGameObjects));
}

std::unique_ptr<EditorCommand> Manager::CreateEditorComponentDeletionCommand(
    GameObject& gameObject,
    Component& component)
{
    std::unique_ptr<Component> detachedComponent = DetachComponentForEditor(gameObject, component);
    if (!detachedComponent)
    {
        return nullptr;
    }

    return std::make_unique<EditorComponentDeletionCommand>(
        gameObject,
        component,
        std::move(detachedComponent));
}
#endif

void Manager::StartLoadedGameObjects(
    std::vector<std::unique_ptr<GameObject>> gameObjects,
    bool assignNewIds)
{
    EnsureGameObjectIds(gameObjects, assignNewIds);

    const size_t firstLoadedIndex = m_GameObjects.size();
    for (std::unique_ptr<GameObject>& gameObject : gameObjects)
    {
        m_GameObjects.emplace_back(std::move(gameObject));
    }

    for (size_t index = firstLoadedIndex; index < m_GameObjects.size(); ++index)
    {
        m_GameObjects[index]->OnLoaded();
        if (m_gameMode == GameMode::Play)
        {
            m_GameObjects[index]->OnStart();
        }
    }
}

bool Manager::SaveCurrentScene()
{
    if (m_gameMode == GameMode::Play)
    {
        Logger::Error("Play中はSceneを保存できません。");
        return false;
    }

    if (!m_currentScene)
    {
        Logger::Error("保存対象のSceneがありません。");
        return false;
    }
    if (!SaveSceneObjects(m_currentScene->GetSceneFilePath()))
    {
        return false;
    }

    ClearCurrentSceneDirty();
    return true;
}

void Manager::MarkCurrentSceneDirty()
{
    if (m_gameMode == GameMode::Edit)
    {
        m_sceneDirty = true;
    }
}

void Manager::ClearCurrentSceneDirty()
{
    m_sceneDirty = false;
}

bool Manager::SaveSceneObjects(const std::filesystem::path& path)
{
    if (m_gameMode == GameMode::Play)
    {
        Logger::Error("Play中はSceneを保存できません。");
        return false;
    }

    std::vector<GameObject*> gameObjects;
    gameObjects.reserve(m_GameObjects.size());
    for (const std::unique_ptr<GameObject>& gameObject : m_GameObjects)
    {
        if (!gameObject->IsDestroyed())
        {
            gameObjects.push_back(gameObject.get());
        }
    }
    return GameObjectYamlSerializer::SaveToFile(
        path,
        path.stem().string(),
        gameObjects,
        PrefabReferenceSaveMode::Include);
}

bool Manager::LoadSceneObjects(const std::filesystem::path& path)
{
    GameObjectLoadResult loaded;
    if (!GameObjectYamlSerializer::LoadFromFile(path, loaded, PrefabSourcePathMode::UseSerializedPath))
    {
        return false;
    }

    // 復元完了後にだけ旧Object群を破棄し、読込失敗時は現在のSceneを維持する。
    DestroyGameObjects();
    if (m_sceneTransitionPhase == SceneTransitionPhase::None)
    {
        StartLoadedGameObjects(std::move(loaded.gameObjects));
    }
    else
    {
        EnsureGameObjectIds(loaded.gameObjects, false);
        for (std::unique_ptr<GameObject>& gameObject : loaded.gameObjects)
        {
            m_pendingGameObjects.emplace_back(std::move(gameObject));
        }
    }
    return true;
}

bool Manager::SavePrefab(GameObject& root, const std::filesystem::path& path)
{
    if (m_gameMode == GameMode::Play)
    {
        Logger::Error("Play中はPrefabを保存できません。");
        return false;
    }

    std::vector<GameObject*> gameObjects;
    std::vector<GameObject*> remaining{ &root };
    while (!remaining.empty())
    {
        GameObject* gameObject = remaining.back();
        remaining.pop_back();
        gameObjects.push_back(gameObject);
        for (TransformBase* child : gameObject->GetTransformBase()->GetChildren())
        {
            remaining.push_back(child->GetOwner());
        }
    }

    if (!GameObjectYamlSerializer::SaveToFile(
        path,
        path.stem().string(),
        gameObjects,
        PrefabReferenceSaveMode::Exclude))
    {
        return false;
    }

    const std::string normalizedPath = path.lexically_normal().generic_string();
    for (GameObject* gameObject : gameObjects)
    {
        gameObject->SetPrefabPath(normalizedPath);
    }
    return true;
}

bool Manager::ApplyToPrefab(GameObject& gameObject)
{
    if (gameObject.GetPrefabPath().empty())
    {
        Logger::Error("Prefab参照を持たないGameObjectはApplyできません。");
        return false;
    }

    GameObject* root = &gameObject;
    while (TransformBase* parent = root->GetTransformBase()->GetParent())
    {
        GameObject* parentObject = parent->GetOwner();
        if (parentObject->GetPrefabPath() != gameObject.GetPrefabPath())
        {
            break;
        }
        root = parentObject;
    }
    return SavePrefab(*root, gameObject.GetPrefabPath());
}

GameObject* Manager::Instantiate(const std::filesystem::path& path)
{
    return InstantiatePrefabInternal(path, nullptr, nullptr, nullptr);
}

GameObject* Manager::Instantiate(
    const std::filesystem::path& path,
    const Vector3& position,
    const Vector3& rotation,
    const Vector3& scale)
{
    return InstantiatePrefabInternal(path, &position, &rotation, &scale);
}

GameObject* Manager::InstantiatePrefabInternal(
    const std::filesystem::path& path,
    const Vector3* position,
    const Vector3* rotation,
    const Vector3* scale)
{
    GameObjectLoadResult loaded;
    if (!GameObjectYamlSerializer::LoadFromFile(path, loaded, PrefabSourcePathMode::UseSourceFilePath))
    {
        return nullptr;
    }

    GameObject* root{};
    for (const std::unique_ptr<GameObject>& gameObject : loaded.gameObjects)
    {
        if (!gameObject->GetTransformBase()->GetParent())
        {
            if (root)
            {
                Logger::Error("PrefabにはRoot GameObjectを1つだけ定義してください: " + path.string());
                return nullptr;
            }
            root = gameObject.get();
        }
    }
    if (!root)
    {
        Logger::Error("PrefabにRoot GameObjectがありません: " + path.string());
        return nullptr;
    }

    root->SetName(path.stem().string() + "(Instance)");
    if (position && rotation && scale)
    {
        if (Transform* transform = root->GetTransform())
        {
            transform->SetPosition(*position);
            transform->SetEulerAngles(*rotation);
            transform->SetScale(*scale);
        }
    }

    if (m_sceneTransitionPhase == SceneTransitionPhase::None)
    {
        StartLoadedGameObjects(std::move(loaded.gameObjects), true);
    }
    else
    {
        EnsureGameObjectIds(loaded.gameObjects, true);
        for (std::unique_ptr<GameObject>& gameObject : loaded.gameObjects)
        {
            m_pendingGameObjects.emplace_back(std::move(gameObject));
        }
    }
    return root;
}


void Manager::InitScene()
{
    if (m_currentScene)
    {
        m_sceneTransitionPhase = SceneTransitionPhase::Destroying;
        m_currentScene->Uninit();
        DestroyGameObjects();
        m_postEffects.clear();
        m_currentScene.reset();
    }

    if (m_nextScene)
    {
        m_currentScene = std::move(m_nextScene);

        m_sceneTransitionPhase = SceneTransitionPhase::Initializing;
        m_currentScene->Init();
    }

    m_sceneTransitionPhase = SceneTransitionPhase::None;
    PendingGameObjectsProcess();
    ClearCurrentSceneDirty();
#if GM_ENABLE_EDITOR
    EditorSystem::ClearCommandHistory();
#endif
}

void Manager::ReloadCurrentScene()
{
    if (!m_currentScene)
    {
        return;
    }

    m_sceneTransitionPhase = SceneTransitionPhase::Destroying;
    m_currentScene->Uninit();
    DestroyGameObjects();
    m_postEffects.clear();

    m_sceneTransitionPhase = SceneTransitionPhase::Initializing;
    m_currentScene->Init();
    m_sceneTransitionPhase = SceneTransitionPhase::None;
    PendingGameObjectsProcess();
    ClearCurrentSceneDirty();
#if GM_ENABLE_EDITOR
    EditorSystem::ClearCommandHistory();
#endif
}

bool Manager::EnterPlayMode()
{
    if (m_gameMode == GameMode::Play)
    {
        return true;
    }

    if (!m_currentScene || !SaveCurrentScene())
    {
        return false;
    }

    m_gameMode = GameMode::Play;
    for (size_t index = 0; index < m_GameObjects.size(); ++index)
    {
        if (!m_GameObjects[index]->IsDestroyed())
        {
            m_GameObjects[index]->OnStart();
        }
    }
    return true;
}

std::filesystem::path Manager::GetCurrentSceneFilePath()
{
    return m_currentScene ? m_currentScene->GetSceneFilePath() : std::filesystem::path{};
}

bool Manager::StopPlayMode()
{
    if (m_gameMode == GameMode::Edit)
    {
        return true;
    }

    // 描画途中のRendererを破棄しないよう、次のUpdate冒頭で処理する。
    m_stopPlayRequested = true;
    return true;
}

#if GM_ENABLE_EDITOR
namespace
{
    class SerializedScene final : public Scene
    {
        std::filesystem::path m_path;

    public:
        explicit SerializedScene(std::filesystem::path path)
            : m_path(std::move(path))
        {
        }

        const std::filesystem::path& GetSceneFilePath() const override
        {
            return m_path;
        }

        void Init() override
        {
            Manager::LoadSceneObjects(m_path);
        }
    };
}

bool Manager::LoadEditorScene(const std::filesystem::path& path)
{
    if (m_gameMode == GameMode::Play || path.extension() != ".scene" || !std::filesystem::exists(path))
    {
        return false;
    }

    m_nextScene = std::make_unique<SerializedScene>(path);
    return true;
}
#endif

bool Manager::Init()
{
    Renderer::Init();
    TextureManager::Init();
    if (!ShaderManager::Init())
    {
        Logger::Fatal("既定シェーダーの読み込みに失敗しました。\nShader: Assets/Shaders/DefaultLit.shader");

        ShaderManager::Uninit();
        TextureManager::Uninit();
        Renderer::Uninit();
        return false;
    }
    if (!MaterialManager::Init())
    {
        Logger::Fatal("Fallback Materialの初期化に失敗しました。");

        MaterialManager::Uninit();
        ShaderManager::Uninit();
        TextureManager::Uninit();
        Renderer::Uninit();
        return false;
    }
    ModelManager::Init();
	Input::Init();
    PostProcess::Init();
    Audio::InitMaster();

#if GM_ENABLE_EDITOR
    // ImGuiの初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Editor専用フォントを先に登録し、初回のDX11フォントアトラス生成へ反映する。
    ImGuiStyleConfig::Load("config/imgui_style.toml");

    const std::filesystem::path& imguiFontPath = ImGuiStyleConfig::GetFontPath();
    const std::u8string imguiFontPathUtf8 = imguiFontPath.u8string();
    std::string imguiFontPathString;
    imguiFontPathString.reserve(imguiFontPathUtf8.size());
    for (const char8_t character : imguiFontPathUtf8)
    {
        imguiFontPathString.push_back(static_cast<char>(character));
    }
    constexpr float imguiFontSize = 18.0f;
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 0;
    fontConfig.OversampleV = 0;

    ImFont* font = io.Fonts->AddFontFromFileTTF(
        imguiFontPathString.c_str(),
        imguiFontSize,
        &fontConfig,
        io.Fonts->GetGlyphRangesJapanese());
    if (font)
    {
        io.FontDefault = font;
    }
    else
    {
        Logger::Warning("ImGuiフォントの読み込みに失敗しました。既定フォントを使用します: " + imguiFontPathString);
    }

    ImGui_ImplWin32_Init(GetWindow());
    ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetDeviceContext());

    // 入力
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Tab / 矢印 / Space / Enter
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // ゲームパッド
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // ImGuiウィンドウのDocking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // MainWindow外のプラットフォームウィンドウ
    io.ConfigWindowsMoveFromTitleBarOnly = true; // 中身をドラッグして誤移動しにくくする

    io.IniFilename = ImGuiStyleConfig::GetIniPathString().c_str(); // 設定ファイルの保存先を指定

    ImGuiStyleConfig::Apply();
#endif

    m_nextScene = std::make_unique<TitleScene>();
#if !GM_ENABLE_EDITOR
    InitScene();
#endif

    return true;
}


void Manager::Uninit()
{
#if GM_ENABLE_EDITOR
    EditorSystem::ClearCommandHistory();
#endif

    m_sceneTransitionPhase = SceneTransitionPhase::Destroying;
    if (m_currentScene)
    {
        m_currentScene->Uninit();
    }
    DestroyGameObjects();
    m_currentScene.reset();
    m_nextScene.reset();
    m_sceneTransitionPhase = SceneTransitionPhase::None;

#if GM_ENABLE_EDITOR
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif

    // PostProcessはMaterialを参照するため、MaterialManagerより先に破棄する。
    m_postEffects.clear();
    PostProcess::Uninit();
    ModelManager::Uninit();
    MaterialManager::Uninit();
    FontManager::Clear();
    TextureManager::Uninit();
    ShaderManager::Uninit();
    Renderer::Uninit();
    Input::Uninit();
    Audio::UninitMaster();
    m_Renderers.clear();

}
void Manager::Update()
{
    PerformanceMonitor::BeginUpdate();

	Input::Update();

    if (m_stopPlayRequested)
    {
        m_stopPlayRequested = false;
        // Play中に予約されたScene遷移は、編集対象Sceneへ戻るため破棄する。
        m_nextScene.reset();
        m_gameMode = GameMode::Edit;
        ReloadCurrentScene();
    }

#if GM_ENABLE_EDITOR
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

    // DockSpaceは各EditorWindowを描く前に構築しておく。
    ImGui::DockSpaceOverViewport(
        0,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode);

	if (EditorSystem::ApplyGameWindowRenderTargetSize())
	{
		for (const std::unique_ptr<PostProcess>& postEffect : m_postEffects)
		{
			postEffect->Resize(Renderer::GetGameWidth(), Renderer::GetGameHeight());
		}
	}
#endif

    if (m_nextScene)
    {
#if GM_ENABLE_EDITOR
        if (EditorSystem::IsGameWindowReady())
#endif
        InitScene();
    }
    else
    {
        PendingGameObjectsProcess();
    }

    if (m_gameMode == GameMode::Play && m_currentScene)
    {
        m_currentScene->Update();
    }

    if (m_gameMode == GameMode::Play)
    {
        for (size_t i = 0; i < m_GameObjects.size(); ++i)
        {
            if (!m_GameObjects[i]->IsDestroyed())
            {
                m_GameObjects[i]->Update();
            }
        }

        // Manager::Update全体からLateUpdateだけを分離して計測する。
        PerformanceMonitor::BeginLateUpdate();
        for (size_t i = 0; i < m_GameObjects.size(); ++i)
        {
            if (!m_GameObjects[i]->IsDestroyed())
            {
                m_GameObjects[i]->LateUpdate();
            }
        }
        PerformanceMonitor::EndLateUpdate();
    }
#if GM_ENABLE_EDITOR
    else if (EditorSystem::IsUIDisplayEnabled())
    {
        // Edit時のUIプレビューでは、Canvasだけを更新して子UITransformのLayoutを導出する。
        for (GameObject* canvasObject : GetObjectsByComponent<UICanvas>())
        {
            if (UICanvas* canvas = canvasObject->GetComponent<UICanvas>())
            {
                canvas->LateUpdate();
            }
        }
    }
#endif

    // 破棄されたオブジェクトの削除
    for (size_t i = 0; i < m_GameObjects.size();)
    {
        if (m_GameObjects[i]->IsDestroyed())
        {
            m_GameObjects[i]->OnDestroy();

            // 末尾と入れ替え
            std::swap(m_GameObjects[i], m_GameObjects.back());

            // 末尾削除
            m_GameObjects.pop_back();

            // インクリメントしない
            // swap後に新しい要素が i に来るので再チェック
        }
        else
        {
            ++i;
        }
    }

    PerformanceMonitor::SetObjectCounts(m_GameObjects.size(), m_pendingGameObjects.size());
    PerformanceMonitor::EndUpdate();
}

void Manager::Draw()
{
	PerformanceMonitor::BeginRender();
	Renderer::Begin();

    if (Camera* camera = Camera::GetMainCamera())
    {
        camera->SetViewProjection();
    }

	LIGHT light{};
	Vector3 lightDirection = Vector3(1.0f, -1.0f, 1.0f).Normalized();

	light.Enable = TRUE;
	light.Direction = XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
	light.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	Renderer::SetLight(light);

    if (m_currentScene)
    {
        m_currentScene->Draw();
    }

    for( auto& renderer : m_Renderers)
    {
        if (renderer->IsOwnerStarted())
        {
            renderer->Submit();
        }
    }
	//for(auto& gameObject : m_GameObjects)
	//{
	//	gameObject->Draw();
	//}

	Renderer::Draw();

    for (int i = 0; i < m_postEffects.size(); ++i)
    {
        m_postEffects[i]->Apply();
    }

#if GM_ENABLE_EDITOR
    EditorSystem::DrawImGui();

	Renderer::BeginEditorFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	Renderer::End();

#if GM_ENABLE_EDITOR
    // Dock解除したウィンドウは、各プラットフォームのネイティブウィンドウへ描画する。
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif

	// Presentの待機時間もフレームの描画負荷として扱う。
	PerformanceMonitor::EndRender();
}
