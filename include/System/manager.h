#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include "math/vector3.h"
#include "System/GameObject.h"
#include "Component/PostEffect.h"

class Camera;
class Component;
class RendererBase;
class Scene;

class Manager
{
private:
	static std::vector<std::unique_ptr<GameObject>> m_GameObjects;
    static std::vector<std::unique_ptr<GameObject>> m_pendingGameObjects;   // オブジェクト追加用の一時的な格納場所
    static std::vector<std::unique_ptr<PostProcess>> m_postEffects;         // ポストエフェクトのリスト
    static std::vector<RendererBase*> m_Renderers;                          // 描画用のレンダラーリスト

    static std::unordered_map<std::string,
        std::function<GameObject* (const Vector3&, const Vector3&, const Vector3&)>> m_FactoryMap;

    static std::unique_ptr<Scene> m_currentScene;
    static std::unique_ptr<Scene> m_nextScene;


    static void PendingGameObjectsProcess()
    {
        for (auto& pendingObj : m_pendingGameObjects)
        {
            m_GameObjects.emplace_back(std::move(pendingObj));
        }
        m_pendingGameObjects.clear();
    }

    static void InitScene();

public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();

    static void RegisterFactory(
        const std::string& typeName,
        std::function<GameObject* (const Vector3&, const Vector3&, const Vector3&)> factoryFunction)
    {
        m_FactoryMap[typeName] = factoryFunction;
    }

    static void RegisterRenderer(RendererBase* renderer)
    {
        m_Renderers.push_back(renderer);
    }

    static void UnregisterRenderer(RendererBase* renderer)
    {
        // スワップポップで効率的に削除
        for(size_t i = 0; i < m_Renderers.size(); ++i)
        {
            if(m_Renderers[i] == renderer)
            {
                std::swap(m_Renderers[i], m_Renderers.back());
                break;
            }
        }
        m_Renderers.pop_back();
    }

    // prefabから生成のみを行う関数（シーンから呼び出し）
    // シーン初期化時の生成でも、Instantiateを呼び出すとOnStartが呼ばれてしまうため、OnStartを呼び出さない生成関数を用意
    static GameObject* InstantiateForBeginScene(
        const std::string& prefabName,
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale)
    {
        auto it = m_FactoryMap.find(prefabName);
        if (it != m_FactoryMap.end())
        {
            GameObject* newObj = it->second(position, rotation, scale);
            m_pendingGameObjects.emplace_back(newObj);

            return newObj;
        }
        return nullptr; // タイプが見つからない場合
    }


    static GameObject* Instantiate(
        const std::string& prefabName,
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale)
    {
        auto it = m_FactoryMap.find(prefabName);
        if (it != m_FactoryMap.end())
        {
            GameObject* newObj = it->second(position, rotation, scale);
            m_pendingGameObjects.emplace_back(newObj);

            newObj->OnStart(); // 生成と同時にOnStartを呼び出す

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
        return m_pendingGameObjects.back().get();
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
            if (gameObject->GetComponent<T>() != nullptr)
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
            if (gameObject->GetComponent<T>() != nullptr)
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
