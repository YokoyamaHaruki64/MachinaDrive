#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include "math/Vector3.h"
#include "component.h"
#include "Component/TransformBase.h"

class Transform;
class UITransform;
class GameObjectReference;

enum class GameObjectTransformType
{
    World,
    UI,
};

class GameObject
{
protected:
    uint64_t m_id = 0;
    std::string m_name{ "Empty" };
    std::string m_prefabPath;
    bool m_isPrefabResolutionError = false;
    TransformBase* m_transformBase = nullptr;
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
        const Vector3& scale = Vector3::One(),
        GameObjectTransformType transformType = GameObjectTransformType::World);
    ~GameObject();

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

    void SetActive(bool active);

    template <typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
        if constexpr (std::is_base_of_v<TransformBase, T>)
        {
            if (m_transformBase)
            {
                return nullptr;
            }
        }

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
    UITransform* GetUITransform() const;
    TransformBase* GetTransformBase() const { return m_transformBase; }
    GameObject* GetParent() const;
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
    // Physicsが作ったHitInfoを、受信対象ObjectのComponent列へ転送する。
    void NotifyHitBegin(const HitInfo& hitInfo)
    {
        NotifyHit([&hitInfo](Component& component) { component.OnHitBegin(hitInfo); });
    }

    void NotifyHit(const HitInfo& hitInfo)
    {
        NotifyHit([&hitInfo](Component& component) { component.OnHit(hitInfo); });
    }

    void NotifyHitEnd(const HitInfo& hitInfo)
    {
        NotifyHit([&hitInfo](Component& component) { component.OnHitEnd(hitInfo); });
    }

    template <typename Callback>
    void NotifyHit(Callback&& callback)
    {
        if (!m_isActive || m_isDestroyed)
        {
            return;
        }

        // 通知中に追加されたComponentは、今回のイベント対象に含めない。
        // Destroy要求後は残りのComponentを呼ばず、既存のLifecycle通知と同じ規則にする。
        const size_t componentCount = m_components.size();
        for (size_t i = 0; i < componentCount && !m_isDestroyRequested; ++i)
        {
            Component* component = m_components[i].get();
            if (component->IsEnabled())
            {
                callback(*component);
            }
        }
    }

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
    void RegisterTransformBase(TransformBase* transformBase) { m_transformBase = transformBase; }

    friend class GameObjectReference;
    friend class Manager;
    friend class TransformBase;
    friend class Physics;
};

// テンプレート関数インクルード（循環参照回避）
#include "component.inl"
