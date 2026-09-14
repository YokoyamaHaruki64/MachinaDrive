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
class UITransform;

// リザルト演出完了後にCanvasを表示し、リトライとタイトル復帰を受け付ける。
class ResultUI final : public Script
{
    GM_SCRIPT(ResultUI, "Result UI")
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

    enum class ResultUIState
    {
        Hidden,
        Waiting,
        Revealing,
        Interactive,
    };

    struct RevealElement
    {
        UITransform* transform{};
        Vector2 originalScale{ 1.0f, 1.0f };
    };

    GameObjectReference m_resultCanvas;
    GameObjectReference m_resultPanel;
    std::vector<GameObjectReference> m_uiObjects;
    GameObjectReference m_timeText;
    GameObjectReference m_maxWaveText;
    GameObjectReference m_maxComboText;
    GameObjectReference m_destroyEnemyText;
    std::vector<UIElement> m_uiElements;
    Material* m_hoverLineMaterial{};
    Audio* m_hoverAudio{};
    Audio* m_clickAudio{};
    ResultUIState m_state = ResultUIState::Hidden;
    float m_displayDelay = 1.0f;
    float m_displayDuration = 0.5f;
    float m_stateElapsedTime{};
    std::vector<RevealElement> m_revealElements;

    void BeginDisplay();
    void SetHierarchyActive(GameObject* object, bool active);
    void CollectRevealElements();
    void SetRevealProgress(float progress);
    void SetResultTexts();
    void SetText(const GameObjectReference& textObject, const std::string& value) const;
    void ApplyHoverVisual(UIElement& element, bool hovered);
    void UpdateElement(UIElement& element);
    void HandleClick(const UIElement& element);

public:
    void OnStart() override;
    void Update() override;
};
