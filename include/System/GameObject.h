#pragma once

#include <vector>
#include <memory>
#include "math/Vector3.h"
#include "component.h"

class Transform;

class GameObject
{
protected:
    Transform* m_transform = nullptr;
	std::vector<std::unique_ptr<Component>> m_components;

    bool m_isDestroy = false;
public:
    GameObject();
    GameObject(
        const Vector3& position,
        const Vector3& rotation = Vector3::Zero(),
        const Vector3& scale = Vector3::One());

    void Destroy()
    {
        m_isDestroy = true;
    }

    bool IsDestroyed() const
    {
        return m_isDestroy;
    }

    template <typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
        m_components.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...));
		T* componentPtr = static_cast<T*>(m_components.back().get());
        componentPtr->OnStart();
		return componentPtr;
	}

    template <typename T, typename... Args>
    T* AddComponentForCreate(Args&&... args)
    {
        m_components.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...));
        T* componentPtr = static_cast<T*>(m_components.back().get());
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
        for (auto& component : m_components)
        {
            component->OnDestroy();
        }

    }

    void OnStart()
    {
        for( size_t i = 0; i < m_components.size(); ++i)
        {
            m_components[i]->OnStart();
        }
    }

	void Update()
	{
        for (size_t i = 0; i < m_components.size(); ++i)
		{
            m_components[i]->Update();
		}
	}

    void LateUpdate()
    {
        for (size_t i = 0; i < m_components.size(); ++i)
        {
            m_components[i]->LateUpdate();
        }
    }
	//void Draw()
	//{
	//	for (auto& component : m_components)
	//	{
	//		component->Draw();
	//	}
	//}

};

// テンプレート関数インクルード（循環参照回避）
#include "component.inl"
