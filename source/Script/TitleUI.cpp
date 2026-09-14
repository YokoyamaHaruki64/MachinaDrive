#include "pch.h"

#include "Component/ComponentInc.h"

#include "Graphics/MaterialManager.h"
#include "Scene/GameScene.h"
#include "Scene/TutorialScene.h"
#include "System/Core/manager.h"
#include "Script/TItleUI.h"
#include "Script/SceneTransition.h"
#include "Script/UI/UIOffset.h"

#include <algorithm>

FIELDS_BEGIN(TitleUI)
    GM_FIELD(m_uiObjects);
    GM_FIELD(m_clickAudioGain);
FIELDS_END()

namespace
{
    constexpr float ButtonSlideDuration = 0.2f;
    constexpr const char* HoverSoundPath = "Assets/Local/Sounds/SE_HoverButton.mp3";
    constexpr const char* ClickSoundPath = "Assets/Local/Sounds/SE_ClickButton.mp3";
}

void TitleUI::OnStart()
{
    Input::SetCursorLock(false);
    Input::SetCursorVisible(true);

    m_uiElements.clear();
    m_hoverLineMaterial = MaterialManager::Load("Assets/Materials/HUDFrameMask.material");
    m_hoverAudio = m_owner->AddComponent<Audio>(HoverSoundPath);
    m_clickAudio = m_owner->AddComponent<Audio>(ClickSoundPath);
    m_clickAudio->SetGain(m_clickAudioGain);

    for (const GameObjectReference& uiObjectRef : m_uiObjects)
    {
        GameObject* const object = uiObjectRef.Get();

        UIElement element;
        const auto collectComponents = [&element](GameObject* candidate)
        {
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
                collectComponents(child->GetOwner());
            }

        }

        if (!element.offsetComponent || !element.buttonComponent || !element.lineImage)
        {
            continue;
        }

        element.originalLineMaterial = element.lineImage->GetMaterial();
        element.originalLineColor = element.lineImage->GetColor();
        element.offsetComponent->SetProgress(0.0f);
        m_uiElements.push_back(element);
    }
}

void TitleUI::ApplyHoverVisual(UIElement& element, bool hovered)
{
    if (!element.lineImage)
    {
        return;
    }

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

void TitleUI::UpdateElement(UIElement& element)
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

    const float progressStep = std::min(
        Time::GetUnscaledDeltaTime() / ButtonSlideDuration,
        1.0f);
    const float currentProgress = element.offsetComponent->GetProgress();
    const float nextProgress = hovered
        ? std::min(currentProgress + progressStep, 1.0f)
        : std::max(currentProgress - progressStep, 0.0f);
    element.offsetComponent->SetProgress(nextProgress);
}

void TitleUI::HandleClick(const UIElement& element)
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
    if (buttonName == "Start Button")
    {
        SceneTransition::ChangeScene<GameScene>(0.8f, EaseType::InOutQuad);
    }
    else if (buttonName == "Tutorial Button")
    {
        SceneTransition::ChangeScene<TutorialScene>(0.8f, EaseType::InOutQuad);
    }
    else if (buttonName == "Exit Button")
    {
        Manager::Quit();
    }
}

void TitleUI::Update()
{
    for (UIElement& element : m_uiElements)
    {
        UpdateElement(element);
        HandleClick(element);
    }
}
