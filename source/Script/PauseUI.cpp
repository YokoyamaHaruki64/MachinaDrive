#include "pch.h"

#include "Component/ComponentInc.h"

#include "Graphics/MaterialManager.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "Script/Pause.h"
#include "Script/PauseUI.h"
#include "Script/SceneTransition.h"
#include "Script/UI/UIOffset.h"
#include "System/Core/manager.h"

#include <algorithm>

namespace
{
    constexpr float ButtonSlideDuration = 0.2f;
    constexpr const char* HoverSoundPath = "Assets/Local/Sounds/SE_HoverButton.mp3";
    constexpr const char* ClickSoundPath = "Assets/Local/Sounds/SE_ClickButton.mp3";
}

FIELDS_BEGIN(PauseUI)
    GM_FIELD(m_pauseCanvas);
    GM_FIELD(m_uiObjects);
FIELDS_END()

void PauseUI::OnStart()
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

    SetPauseActive(false);
}

void PauseUI::SetPauseActive(bool active)
{
    Pause::SetPause(active);
    SetHierarchyActive(m_pauseCanvas.Get(), active);

    Input::SetCursorVisible(active);
    Input::SetCursorLock(!active);
}

void PauseUI::SetHierarchyActive(GameObject* object, bool active)
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

void PauseUI::ApplyHoverVisual(UIElement& element, bool hovered)
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

void PauseUI::UpdateElement(UIElement& element)
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

void PauseUI::HandleClick(const UIElement& element)
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
    if (buttonName == "Pause Exit Button")
    {
        SetPauseActive(false);
    }
    else if (buttonName == "Retry Button")
    {
        SetPauseActive(false);
        SceneTransition::ChangeScene<GameScene>(0.8f, EaseType::InOutQuad);
    }
    else if (buttonName == "Return Title Button")
    {
        SetPauseActive(false);
        SceneTransition::ChangeScene<TitleScene>(0.8f, EaseType::InOutQuad);
    }
}

void PauseUI::Update()
{
#if GM_ENABLE_EDITOR

    if (Input::GetKeyTrigger(Input::KeyCode::F1))
        SetPauseActive(!Pause::IsPause());

#else

    if (Input::GetKeyTrigger(Input::KeyCode::ESCAPE))
        SetPauseActive(!Pause::IsPause());

#endif // GM_ENABLE_EDITOR

    if (!Pause::IsPause())
    {
        return;
    }

    for (UIElement& element : m_uiElements)
    {
        UpdateElement(element);
        HandleClick(element);
    }
}
