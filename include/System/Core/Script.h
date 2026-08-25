#pragma once

#include "System/Core/component.h"

// ゲーム固有の振る舞いを記述するComponentの基底クラス。
class Script : public Component
{
public:
    Script() = delete;
    explicit Script(GameObject* owner)
        : Component(owner)
    {
    }

    ~Script() override = default;
};

#include "System/Core/ComponentRegistry.h"
