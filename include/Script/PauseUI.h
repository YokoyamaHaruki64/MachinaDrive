#pragma once

#include "Common/Color.h"
#include "System/Core/GameObjectReference.h"
#include "System/Core/Script.h"

#include <vector>

class Audio;
class Material;
class UIButton;
class UIImage;
class UIOffset;

// ポーズ画面の表示と、時間停止中のUI入力を管理する。
class PauseUI final : public Script
{
    GM_SCRIPT(PauseUI, "Pause UI")
    USE_FIELDS

    struct UIElement
    {
        UIOffset* offsetComponent{};
        UIButton* buttonComponent{};
        UIImage* lineImage{};
        Material* originalLineMaterial{};
        Color originalLineColor{};
        bool isHovered{};
    };

    GameObjectReference m_pauseCanvas;
    std::vector<GameObjectReference> m_uiObjects;
    std::vector<UIElement> m_uiElements;
    Material* m_hoverLineMaterial{};
    Audio* m_hoverAudio{};
    Audio* m_clickAudio{};

    void SetPauseActive(bool active);
    void SetHierarchyActive(GameObject* object, bool active);
    void ApplyHoverVisual(UIElement& element, bool hovered);
    void UpdateElement(UIElement& element);
    void HandleClick(const UIElement& element);

public:
    void OnStart() override;
    void Update() override;
};
