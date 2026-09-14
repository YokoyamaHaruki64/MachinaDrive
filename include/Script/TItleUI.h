#pragma once

#include "Common/Color.h"
#include "System/Core/GameObjectReference.h"
#include "System/Core/Script.h"

#include <vector>

class Material;
class Audio;
class UIButton;
class UIImage;
class UIOffset;

class TitleUI : public Script
{
    GM_SCRIPT(TitleUI, "Title UI")
    USE_FIELDS
private:
    struct UIElement
    {
        UIOffset* offsetComponent{};
        UIButton* buttonComponent{};
        UIImage* lineImage{};
        Material* originalLineMaterial{};
        Color originalLineColor{};
        bool isHovered{};
    };

    std::vector<GameObjectReference> m_uiObjects;
    std::vector<UIElement> m_uiElements;
    Material* m_hoverLineMaterial{};
    Audio* m_hoverAudio{};
    Audio* m_clickAudio{};

    float m_clickAudioGain = 1.0f;

    void ApplyHoverVisual(UIElement& element, bool hovered);
    void UpdateElement(UIElement& element);
    void HandleClick(const UIElement& element);

public:
    void OnStart() override;
    void Update() override;
};
