#pragma once

#include "math/Vector3.h"
#include "System/Editor/FieldRegistry.h"

class Transform;
class GameObject;
struct HitInfo;

enum class TransformChange : uint8_t
{
    None = 0,
    Position = 1 << 0,
    Rotation = 1 << 1,
    Scale = 1 << 2,
    Hierarchy = 1 << 3,
};

constexpr TransformChange operator|(TransformChange left, TransformChange right)
{
    return static_cast<TransformChange>(
        static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
}

constexpr bool HasTransformChange(TransformChange value, TransformChange flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

struct TransformChangedEvent
{
    Transform* source{ nullptr };
    TransformChange changes{ TransformChange::None };
};

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
    // GameObject自身が無効になった遷移で呼び出される。親の状態変更では呼ばない。
    virtual void OnInactive() {};
    // Componentが有効になった遷移で呼び出される。
    virtual void OnEnable() {};
    // Componentが無効になった遷移、または所有GameObjectが無効になった際に呼び出される。
    virtual void OnDisable() {};
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

private:
    // Engine内部の同期通知であり、通常のScript lifecycle APIには公開しない。
    virtual void OnTransformChanged(const TransformChangedEvent&) {}

    friend class GameObject;
};
