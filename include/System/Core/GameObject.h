#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "math/Vector3.h"
#include "component.h"

class Transform;
class GameObjectReference;
enum class ParentTransformMode;

class GameObject
{
protected:
    uint64_t m_id = 0;
    std::string m_name{ "Empty" };
    std::string m_prefabPath;
    bool m_isPrefabResolutionError = false;
    Transform* m_transform = nullptr;
	std::vector<std::unique_ptr<Component>> m_components;
    std::vector<GameObjectReference*> m_references;

    bool m_isStarted = false;
    bool m_isActive = true;
    bool m_isDestroyRequested = false;
    bool m_isDestroyed = false;
public:
    GameObject();
    GameObject(
        const Vector3& position,
        const Vector3& rotation = Vector3::Zero(),
        const Vector3& scale = Vector3::One());
    GameObject(
        std::string name,
        const Vector3& position = Vector3::Zero(),
        const Vector3& rotation = Vector3::Zero(),
        const Vector3& scale = Vector3::One());

    uint64_t GetId() const
    {
        return m_id;
    }

    void SetId(uint64_t id)
    {
        m_id = id;
    }

    const std::string& GetName() const
    {
        return m_name;
    }

    void SetName(std::string name)
    {
        m_name = std::move(name);
    }

    const std::string& GetPrefabPath() const
    {
        return m_prefabPath;
    }

    void SetPrefabPath(std::string path)
    {
        m_prefabPath = std::move(path);
        m_isPrefabResolutionError = false;
    }

    void MarkPrefabResolutionError(std::string path)
    {
        m_prefabPath = std::move(path);
        m_isPrefabResolutionError = true;
    }

    bool IsPrefabResolutionError() const
    {
        return m_isPrefabResolutionError;
    }

    void Destroy();

    bool IsDestroyed() const
    {
        return m_isDestroyRequested || m_isDestroyed;
    }

    bool IsStarted() const
    {
        return m_isStarted;
    }

    bool IsActive() const
    {
        return m_isActive;
    }

    void SetActive(bool active)
    {
        if (m_isActive == active)
        {
            return;
        }

        m_isActive = active;
        if (!m_isActive || !m_isStarted || m_isDestroyRequested || m_isDestroyed)
        {
            return;
        }

        NotifyActiveState();
    }

    template <typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
        m_components.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...));
        T* componentPtr = static_cast<T*>(m_components.back().get());
        if (m_isStarted)
        {
            componentPtr->OnLoaded();
            componentPtr->OnStart();
            if (m_isActive && !m_isDestroyRequested && !m_isDestroyed)
            {
                componentPtr->OnActive();
                if (componentPtr->IsEnabled())
                {
                    componentPtr->OnEnable();
                }
            }
        }

        return componentPtr;
    }

	template <typename T>
	T* GetComponent()
	{
		for (const auto& component : m_components)
		{
			if (T* casted = dynamic_cast<T*>(component.get()))
			{
				return casted;
			}
		}
		return nullptr;
	}

    Transform* GetTransform() const;
    bool SetParent(GameObject* parent, ParentTransformMode mode);
    bool SetParent(GameObject* parent);

    std::vector<Component*> GetComponents() const
    {
        std::vector<Component*> componentPtrs;
        for (const auto& component : m_components)
        {
            componentPtrs.push_back(component.get());
        }
        return componentPtrs;
    }

    void OnDestroy()
    {
        if (m_isDestroyed)
        {
            return;
        }

        m_isDestroyed = true;
        m_isStarted = false;

        ClearReferences();

        const size_t componentCount = m_components.size();
        for (size_t i = 0; i < componentCount; ++i)
        {
            m_components[i]->OnDestroy();
        }
    }

    void OnLoaded()
    {
        if (m_isDestroyRequested || m_isDestroyed)
        {
            return;
        }

        for (const std::unique_ptr<Component>& component : m_components)
        {
            component->OnLoaded();
        }
    }

    void OnStart()
    {
        if (m_isStarted || m_isDestroyRequested || m_isDestroyed)
        {
            return;
        }

        m_isStarted = true;

        // OnStart中に追加されたComponentはAddComponent側で開始する。
        const size_t componentCount = m_components.size();
        for (size_t i = 0; i < componentCount; ++i)
        {
            m_components[i]->OnStart();
        }

        if (m_isActive)
        {
            NotifyActiveState();
        }
    }

    void OnEditorUpdate()
    {
        if (!m_isActive || m_isDestroyRequested || m_isDestroyed)
        {
            return;
        }

        for (const std::unique_ptr<Component>& component : m_components)
        {
            if (component->IsEnabled())
            {
                component->OnEditorUpdate();
            }
        }
    }

	void Update()
	{
        if (!m_isActive)
        {
            return;
        }

        for (size_t i = 0; i < m_components.size() && !m_isDestroyRequested; ++i)
		{
            if (m_components[i]->IsEnabled())
            {
                m_components[i]->Update();
            }
		}
	}

    void LateUpdate()
    {
        if (!m_isActive)
        {
            return;
        }

        for (size_t i = 0; i < m_components.size() && !m_isDestroyRequested; ++i)
        {
            if (m_components[i]->IsEnabled())
            {
                m_components[i]->LateUpdate();
            }
        }
    }
	//void Draw()
	//{
	//	for (auto& component : m_components)
	//	{
	//		component->Draw();
	//	}
	//}

private:
    void NotifyActiveState()
    {
        const size_t componentCount = m_components.size();
        for (size_t i = 0; i < componentCount && !m_isDestroyRequested; ++i)
        {
            Component* component = m_components[i].get();
            component->OnActive();
            if (component->IsEnabled())
            {
                component->OnEnable();
            }
        }
    }

    void RegisterReference(GameObjectReference* reference);
    void UnregisterReference(GameObjectReference* reference);
    void ClearReferences();

    friend class GameObjectReference;
    friend class Manager;
};

// テンプレート関数インクルード（循環参照回避）
#include "component.inl"
