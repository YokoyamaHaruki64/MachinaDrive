#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include <utility>
#include "math/vector3.h"
#include "System/GameObject.h"
#include "Component/PostEffect.h"

class Camera;
class Component;
class RendererBase;
class Scene;

class Manager
{
public:
    using PrefabFactory = std::function<std::unique_ptr<GameObject>(
        const Vector3&,
        const Vector3&,
        const Vector3&)>;

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

    static std::unordered_map<std::string, PrefabFactory> m_FactoryMap;

    static std::unique_ptr<Scene> m_currentScene;
    static std::unique_ptr<Scene> m_nextScene;
    static SceneTransitionPhase m_sceneTransitionPhase;


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
                m_GameObjects[i]->OnStart();
            }
        }
    }

    static void InitScene();
    static void DestroyGameObjects();

public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();

    static void RegisterFactory(
        const std::string& typeName,
        PrefabFactory factoryFunction)
    {
        m_FactoryMap[typeName] = std::move(factoryFunction);
    }

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

    // prefabの構築完了後に開始する。シーン構築中は全オブジェクトをまとめて開始する。
    static GameObject* Instantiate(
        const std::string& prefabName,
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale)
    {
        auto it = m_FactoryMap.find(prefabName);
        if (it != m_FactoryMap.end())
        {
            std::unique_ptr<GameObject> newObject = it->second(position, rotation, scale);
            if (!newObject)
            {
                return nullptr;
            }

            GameObject* newObj = newObject.get();
            m_pendingGameObjects.emplace_back(std::move(newObject));

            if (m_sceneTransitionPhase == SceneTransitionPhase::None)
            {
                newObj->OnStart();
            }

            return newObj;
        }
        return nullptr; // タイプが見つからない場合
    }

    static GameObject* CreateEmpty(
        Vector3 position = Vector3::Zero(),
        Vector3 rotation = Vector3::Zero(),
        Vector3 scale = Vector3::One())
	{
        m_pendingGameObjects.emplace_back(std::make_unique<GameObject>(position, rotation, scale));
        GameObject* newObj = m_pendingGameObjects.back().get();

        if (m_sceneTransitionPhase == SceneTransitionPhase::None)
        {
            newObj->OnStart();
        }

        return newObj;
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
