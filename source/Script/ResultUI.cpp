#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Easing.h"
#include "Graphics/MaterialManager.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Script/Result.h"
#include "Script/ResultUI.h"
#include "Script/SceneTransition.h"
#include "Script/UI/UIOffset.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    constexpr float ButtonSlideDuration = 0.2f;
    constexpr const char* HoverSoundPath = "Assets/Local/Sounds/SE_HoverButton.mp3";
    constexpr const char* ClickSoundPath = "Assets/Local/Sounds/SE_ClickButton.mp3";
}

FIELDS_BEGIN(ResultUI)
    GM_FIELD(m_resultCanvas);
    GM_FIELD(m_resultPanel);
    GM_FIELD(m_uiObjects);
    GM_FIELD(m_timeText);
    GM_FIELD(m_maxWaveText);
    GM_FIELD(m_maxComboText);
    GM_FIELD(m_destroyEnemyText);
    GM_FIELD(m_displayDelay);
    GM_FIELD(m_displayDuration);
FIELDS_END()

void ResultUI::OnStart()
{
    m_hoverLineMaterial = MaterialManager::Load("Assets/Materials/HUDFrameMask.material");
    m_hoverAudio = m_owner->AddComponent<Audio>(HoverSoundPath);
    m_clickAudio = m_owner->AddComponent<Audio>(ClickSoundPath);

    m_uiElements.clear();
    for (const GameObjectReference& uiObjectRef : m_uiObjects)
    {
        GameObject* const object = uiObjectRef.Get();
        if (!object)
        {
            continue;
        }

        UIElement element;
        const auto collectComponents = [&element](GameObject* candidate)
        {
            if (!candidate)
            {
                return;
            }

            if (!element.offsetComponent)
                element.offsetComponent = candidate->GetComponent<UIOffset>();
            if (!element.buttonComponent)
                element.buttonComponent = candidate->GetComponent<UIButton>();
            if (!element.lineImage && candidate->GetName() == "Line")
                element.lineImage = candidate->GetComponent<UIImage>();
        };

        collectComponents(object);
        if (TransformBase* const transform = object->GetTransformBase())
        {
            for (TransformBase* child : transform->GetChildren())
            {
                collectComponents(child ? child->GetOwner() : nullptr);
            }
        }

        if (!element.buttonComponent || !element.lineImage)
        {
            continue;
        }

        element.originalLineMaterial = element.lineImage->GetMaterial();
        element.originalLineColor = element.lineImage->GetColor();
        if (element.offsetComponent)
        {
            element.offsetComponent->SetProgress(0.0f);
        }
        m_uiElements.push_back(element);
    }

    CollectRevealElements();
    SetHierarchyActive(m_resultCanvas.Get(), false);
    m_state = ResultUIState::Hidden;
    m_stateElapsedTime = 0.0f;
    SetRevealProgress(0.0f);
}

void ResultUI::BeginDisplay()
{
    SetHierarchyActive(m_resultCanvas.Get(), true);
    SetResultTexts();
    SetRevealProgress(0.0f);
    m_stateElapsedTime = 0.0f;
    m_state = ResultUIState::Waiting;
}

void ResultUI::SetHierarchyActive(GameObject* object, bool active)
{
    if (!object)
    {
        return;
    }

    if (!active)
    {
        if (TransformBase* const transform = object->GetTransformBase())
        {
            for (TransformBase* child : transform->GetChildren())
            {
                SetHierarchyActive(child ? child->GetOwner() : nullptr, false);
            }
        }
        object->SetActive(false);
        return;
    }

    object->SetActive(true);
    if (TransformBase* const transform = object->GetTransformBase())
    {
        for (TransformBase* child : transform->GetChildren())
        {
            SetHierarchyActive(child ? child->GetOwner() : nullptr, true);
        }
    }
}

void ResultUI::CollectRevealElements()
{
    m_revealElements.clear();

    const auto addRevealElement = [this](GameObject* object)
    {
        if (!object)
        {
            return;
        }

        if (UITransform* const transform = object->GetComponent<UITransform>())
        {
            m_revealElements.push_back({ transform, transform->GetUIScale() });
        }
    };

    addRevealElement(m_resultPanel.Get());
    for (const GameObjectReference& uiObject : m_uiObjects)
    {
        addRevealElement(uiObject.Get());
    }
}

void ResultUI::SetRevealProgress(float progress)
{
    const float easedProgress = Easing::Evaluate(progress, EaseType::OutCubic);
    for (const RevealElement& element : m_revealElements)
    {
        if (element.transform)
        {
            element.transform->SetUIScale(element.originalScale * easedProgress);
        }
    }
}

void ResultUI::SetResultTexts()
{
    const int totalCentiseconds = static_cast<int>(std::round(Result::GetSurvivalTime() * 100.0f));
    const int minutes = totalCentiseconds / 6000;
    const int seconds = (totalCentiseconds / 100) % 60;
    const int centiseconds = totalCentiseconds % 100;

    char timeText[16];
    std::snprintf(timeText, sizeof(timeText), "%02d:%02d.%02d", minutes, seconds, centiseconds);
    SetText(m_timeText, timeText);
    SetText(m_maxWaveText, std::to_string(Result::GetWave()));
    SetText(m_maxComboText, std::to_string(Result::GetMaxCombo()));
    SetText(m_destroyEnemyText, std::to_string(Result::GetDefeatedEnemyCount()));
}

void ResultUI::SetText(const GameObjectReference& textObject, const std::string& value) const
{
    if (GameObject* const object = textObject.Get())
    {
        if (UIText* const text = object->GetComponent<UIText>())
        {
            text->SetText(value);
        }
    }
}

void ResultUI::ApplyHoverVisual(UIElement& element, bool hovered)
{
    if (hovered)
    {
        if (m_hoverLineMaterial)
        {
            element.lineImage->SetMaterial(0, m_hoverLineMaterial);
        }
        element.lineImage->SetColor(Color::White());
        return;
    }

    if (element.originalLineMaterial)
    {
        element.lineImage->SetMaterial(0, element.originalLineMaterial);
    }
    element.lineImage->SetColor(element.originalLineColor);
}

void ResultUI::UpdateElement(UIElement& element)
{
    const UIButtonState state = element.buttonComponent->GetState();
    const bool hovered = state == UIButtonState::Hovered || state == UIButtonState::Pressed;
    if (hovered != element.isHovered)
    {
        element.isHovered = hovered;
        ApplyHoverVisual(element, hovered);
        if (hovered && m_hoverAudio)
        {
            m_hoverAudio->Play();
        }
    }

    if (!element.offsetComponent)
    {
        return;
    }

    const float progressStep = std::min(Time::GetUnscaledDeltaTime() / ButtonSlideDuration, 1.0f);
    const float currentProgress = element.offsetComponent->GetProgress();
    const float nextProgress = hovered
        ? std::min(currentProgress + progressStep, 1.0f)
        : std::max(currentProgress - progressStep, 0.0f);
    element.offsetComponent->SetProgress(nextProgress);
}

void ResultUI::HandleClick(const UIElement& element)
{
    if (!element.buttonComponent->WasClicked())
    {
        return;
    }

    if (m_clickAudio)
    {
        m_clickAudio->Play();
    }

    const std::string& buttonName = element.buttonComponent->GetOwner()->GetName();
    if (buttonName == "Result Retry Button")
    {
        SceneTransition::ChangeScene<GameScene>(0.8f, EaseType::InOutQuad);
    }
    else if (buttonName == "Result Return Title Button")
    {
        SceneTransition::ChangeScene<TitleScene>(0.8f, EaseType::InOutQuad);
    }
}

void ResultUI::Update()
{
    switch (m_state)
    {
    case ResultUIState::Hidden:
    {
        if (Result::GetResultState() == Result::ResultState::Completed)
        {
            BeginDisplay();
        }
        break;
    }

    case ResultUIState::Waiting:
    {
        m_stateElapsedTime += Time::GetUnscaledDeltaTime();
        if (m_stateElapsedTime >= m_displayDelay)
        {
            m_stateElapsedTime = 0.0f;
            m_state = ResultUIState::Revealing;
        }
        break;
    }

    case ResultUIState::Revealing:
    {
        m_stateElapsedTime += Time::GetUnscaledDeltaTime();
        const float revealProgress = m_displayDuration > 0.0f
            ? Math::Clamp(m_stateElapsedTime / m_displayDuration, 0.0f, 1.0f)
            : 1.0f;
        SetRevealProgress(revealProgress);
        if (revealProgress >= 1.0f)
        {
            Input::SetCursorVisible(true);
            Input::SetCursorLock(false);
            m_state = ResultUIState::Interactive;
        }
        break;
    }

    case ResultUIState::Interactive:
    {
        for (UIElement& element : m_uiElements)
        {
            UpdateElement(element);
            HandleClick(element);
        }
        break;
    }
    }
}
