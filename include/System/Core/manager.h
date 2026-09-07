#pragma once

#include <vector>
#include <memory>
#include <filesystem>
#include <cstdint>
#include <string>
#include <utility>
#include "math/vector3.h"
#include "System/Core/GameObject.h"
#include "Component/PostEffect.h"
#include "Component/UICanvas.h"

class Camera;
class Component;
class EditorCommand;
class RendererBase;
class Scene;

class Manager
{
public:
    enum class GameMode
    {
        Edit,
        Play,
    };

private:
    enum class SceneTransitionPhase
    {
        None,
        Destroying,
        Initializing,
    };

	static std::vector<std::unique_ptr<GameObject>> m_GameObjects;
    static std::vector<std::unique_ptr<GameObject>> m_pendingGameObjects;   // オブジェクト追加用の一時的な格納場所
    static std::vector<std::unique_ptr<PostProcess>> m_postEffects;         // ポストエフェクトのリスト
    static std::vector<RendererBase*> m_Renderers;                          // 描画用のレンダラーリスト

    static std::unique_ptr<Scene> m_currentScene;
    static std::unique_ptr<Scene> m_nextScene;
    static SceneTransitionPhase m_sceneTransitionPhase;
    static GameMode m_gameMode;
    static bool m_stopPlayRequested;
    static bool m_sceneDirty;
    static uint64_t m_nextGameObjectId;

    static uint64_t AllocateGameObjectId();
    static void EnsureGameObjectIds(
        std::vector<std::unique_ptr<GameObject>>& gameObjects,
        bool assignNewIds);

#if GM_ENABLE_EDITOR
    struct EditorDetachedGameObjects
    {
        struct Entry
        {
            std::unique_ptr<GameObject> gameObject;
            GameObject* previousParent = nullptr;
        };

        std::vector<Entry> entries;

        bool Empty() const { return entries.empty(); }
    };

    class EditorObjectLifetimeCommand;
    class EditorComponentDeletionCommand;

    static EditorDetachedGameObjects DetachForEditor(const std::vector<GameObject*>& gameObjects);
    static void RestoreForEditor(EditorDetachedGameObjects& detachedGameObjects);
    static void DestroyDetachedForEditor(EditorDetachedGameObjects& detachedGameObjects);
    static std::unique_ptr<Component> DetachComponentForEditor(GameObject& gameObject, Component& component);
    static void RestoreComponentForEditor(GameObject& gameObject, std::unique_ptr<Component>& component);
    static void DestroyDetachedComponentForEditor(std::unique_ptr<Component>& component);
#endif


    static void PendingGameObjectsProcess()
    {
        std::vector<std::unique_ptr<GameObject>> pendingGameObjects = std::move(m_pendingGameObjects);
        const size_t firstPendingIndex = m_GameObjects.size();

        for (auto& pendingObj : pendingGameObjects)
        {
            m_GameObjects.emplace_back(std::move(pendingObj));
        }

        // 検索時に同じ生成バッチの全オブジェクトが見える状態で開始する。
        for (size_t i = firstPendingIndex; i < m_GameObjects.size(); ++i)
        {
            if (!m_GameObjects[i]->IsDestroyed())
            {
                m_GameObjects[i]->OnLoaded();
                if (m_gameMode == GameMode::Play)
                {
                    m_GameObjects[i]->OnStart();
                }
            }
        }
    }

    static void InitScene();
    static void ReloadCurrentScene();
    static void DestroyGameObjects();
    static void StartLoadedGameObjects(
        std::vector<std::unique_ptr<GameObject>> gameObjects,
        bool assignNewIds = false);
    static GameObject* InstantiatePrefabInternal(
        const std::filesystem::path& path,
        const Vector3* position,
        const Vector3* rotation,
        const Vector3* scale);

public:
	static bool Init();
	static void Uninit();
	static void Update();
	static void Draw();

    static GameMode GetGameMode() { return m_gameMode; }
    static bool IsPlayMode() { return m_gameMode == GameMode::Play; }
    static bool IsCurrentSceneDirty() { return m_sceneDirty; }
    static void MarkCurrentSceneDirty();
    static void ClearCurrentSceneDirty();
    static std::filesystem::path GetCurrentSceneFilePath();
    static bool EnterPlayMode();
    static bool StopPlayMode();
#if GM_ENABLE_EDITOR
    static bool LoadEditorScene(const std::filesystem::path& path);
#endif

    static bool SaveCurrentScene();
    static bool SaveSceneObjects(const std::filesystem::path& path);
    static bool LoadSceneObjects(const std::filesystem::path& path);
    static bool SavePrefab(GameObject& root, const std::filesystem::path& path);
    static bool ApplyToPrefab(GameObject& gameObject);
    static GameObject* Instantiate(const std::filesystem::path& path);
    static GameObject* Instantiate(
        const std::filesystem::path& path,
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale);

    static const std::vector<std::unique_ptr<GameObject>>& GetActiveGameObjects() { return m_GameObjects; }
    static std::vector<GameObject*> GetObjectsByTag(const std::string& tag);
    static GameObject* GetObjectFirstByTag(const std::string& tag);

#if GM_ENABLE_EDITOR
    static std::unique_ptr<EditorCommand> CreateEditorCreationCommand(
        const std::vector<GameObject*>& gameObjects);
    static std::unique_ptr<EditorCommand> CreateEditorDeletionCommand(
        const std::vector<GameObject*>& selectedGameObjects);
    static std::unique_ptr<EditorCommand> CreateEditorComponentDeletionCommand(
        GameObject& gameObject,
        Component& component);
#endif

    static void RegisterRenderer(RendererBase* renderer)
    {
        m_Renderers.push_back(renderer);
    }

    static void UnregisterRenderer(RendererBase* renderer)
    {
        for (size_t i = 0; i < m_Renderers.size(); ++i)
        {
            if (m_Renderers[i] == renderer)
            {
                std::swap(m_Renderers[i], m_Renderers.back());
                m_Renderers.pop_back();
                return;
            }
        }
    }

    static GameObject* CreateEmpty(
        Vector3 position = Vector3::Zero(),
        Vector3 rotation = Vector3::Zero(),
        Vector3 scale = Vector3::One())
	{
        return CreateEmpty("Empty", position, rotation, scale);
	}

    static GameObject* CreateEmpty(
        std::string name,
        Vector3 position = Vector3::Zero(),
        Vector3 rotation = Vector3::Zero(),
        Vector3 scale = Vector3::One())
    {
        std::unique_ptr<GameObject> newObject = std::make_unique<GameObject>(std::move(name), position, rotation, scale);
        GameObject* newObj = newObject.get();
        newObj->SetId(AllocateGameObjectId());

        if (m_sceneTransitionPhase == SceneTransitionPhase::None)
        {
            m_GameObjects.emplace_back(std::move(newObject));
            newObj->OnLoaded();
            if (m_gameMode == GameMode::Play)
            {
                newObj->OnStart();
            }

            return newObj;
        }

        m_pendingGameObjects.emplace_back(std::move(newObject));

        return newObj;
    }

    static GameObject* CreateUIObject(std::string name = "UI Object")
    {
        std::unique_ptr<GameObject> newObject = std::make_unique<GameObject>(
            std::move(name),
            Vector3::Zero(),
            Vector3::Zero(),
            Vector3::One(),
            GameObjectTransformType::UI);
        GameObject* newObj = newObject.get();
        newObj->SetId(AllocateGameObjectId());

        if (m_sceneTransitionPhase == SceneTransitionPhase::None)
        {
            m_GameObjects.emplace_back(std::move(newObject));
            newObj->OnLoaded();
            if (m_gameMode == GameMode::Play)
            {
                newObj->OnStart();
            }
            return newObj;
        }

        m_pendingGameObjects.emplace_back(std::move(newObject));
        return newObj;
    }

    static GameObject* CreateUICanvas(std::string name = "Canvas")
    {
        GameObject* const canvasObject = CreateUIObject(std::move(name));
        canvasObject->AddComponent<UICanvas>();
        return canvasObject;
    }

    template<typename T>
    static void ChangeScene()
    {
        m_nextScene = std::make_unique<T>();
    }

    // コンポーネントからゲームオブジェクトを取得するための関数
    template <typename T>
    static GameObject* GetFirstObjectByComponent()
    {
        for (const auto& gameObject : m_GameObjects)
        {
            if (!gameObject->IsDestroyed() && gameObject->GetComponent<T>() != nullptr)
            {
                return gameObject.get();
            }
        }
        return nullptr; // 見つからない場合
	}

    template <typename T>
    static std::vector<GameObject*> GetObjectsByComponent()
    {
        std::vector<GameObject*> result;
        for (const auto& gameObject : m_GameObjects)
        {
            if (!gameObject->IsDestroyed() && gameObject->GetComponent<T>() != nullptr)
            {
                result.push_back(gameObject.get());
            }
        }
        return result; // 見つからない場合は空のベクターを返す
    }

    static void AddPostEffect(std::unique_ptr<PostProcess> postEffect)
    {
        m_postEffects.push_back(std::move(postEffect));
    }

    static void RemovePostEffect(PostProcess* postEffect)
    {
        for (size_t i = 0; i < m_postEffects.size(); ++i)
        {
            if (m_postEffects[i].get() == postEffect)
            {
                std::swap(m_postEffects[i], m_postEffects.back());
                m_postEffects.pop_back();
                break;
            }
        }
    }
};
