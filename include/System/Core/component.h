#pragma once

#include "math/Vector3.h"
#include "System/Editor/FieldRegistry.h"

class Transform;
class GameObject;
struct HitInfo;

class Component
{

protected:
	GameObject* m_owner = nullptr;
    bool m_isEnabled = true;

public:
	Component() = delete;
    explicit Component(GameObject* owner)
        : m_owner(owner)
    {
    }
    virtual ~Component() {}

    virtual void OnLoaded() {};
    virtual void OnEditorUpdate() {};
    virtual void OnStart() {};
    // GameObjectが有効になった遷移で呼び出される。
    virtual void OnActive() {};
    // Componentが有効になった遷移で呼び出される。
    virtual void OnEnable() {};
    virtual void OnDestroy() {};

    // 物理接触の通知。GameObjectが物理ステップ後に有効な全Componentへ転送する。
    virtual void OnHitBegin(const HitInfo&) {};
    virtual void OnHit(const HitInfo&) {};
    virtual void OnHitEnd(const HitInfo&) {};

	virtual void Update() {};
    virtual void LateUpdate() {};
	virtual void Draw() {};

    bool DrawInspectorImGui(
        InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All,
        bool* editStarted = nullptr,
        bool* editFinished = nullptr);
    bool ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context = {});
    bool FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context = {});

    Transform* transform() const;
    GameObject* GetOwner() const { return m_owner; }

    bool IsEnabled() const { return m_isEnabled; }
    void SetEnabled(bool enabled);

    template <typename T>
    T* GetComponent() const;

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
