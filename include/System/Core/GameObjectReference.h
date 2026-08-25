#pragma once

class GameObject;

// GameObjectの寿命を所有せずに参照する。対象がDestroyされると自動でnullptrになる。
class GameObjectReference final
{
private:
    GameObject* m_target = nullptr;
    bool m_isResolutionError = false;

public:
    GameObjectReference() = default;
    explicit GameObjectReference(GameObject* target);
    ~GameObjectReference();

    GameObjectReference(const GameObjectReference& other);
    GameObjectReference& operator=(const GameObjectReference& other);
    GameObjectReference(GameObjectReference&& other) noexcept;
    GameObjectReference& operator=(GameObjectReference&& other) noexcept;
    GameObjectReference& operator=(GameObject* target);

    GameObject* operator->() const;
    GameObject& operator*() const;
    explicit operator bool() const;

    GameObject* Get() const;
    bool IsResolutionError() const;
    void Set(GameObject* target);
    void Clear();
    void MarkResolutionError();
};
