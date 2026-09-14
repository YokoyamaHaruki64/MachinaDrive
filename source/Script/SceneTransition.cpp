#include "pch.h"

#include "Script/SceneTransition.h"

#include "Component/PostEffect.h"
#include "System/Core/GM_Time.h"
#include "System/Rendering/renderer.h"

#include <algorithm>

namespace
{
    constexpr std::string_view TRANSITION_EFFECT_NAME{ "HorizontalGlitchTransition" };
    constexpr std::string_view PROGRESS_PROPERTY_NAME{ "_Progress" };
}

void SceneTransition::SetEffectProgress()
{
    PostProcess* effect = Renderer::FindPostEffect(TRANSITION_EFFECT_NAME);
    if (effect)
    {
        effect->GetImpulseOverride().SetFloat(PROGRESS_PROPERTY_NAME, s_progress);
    }
}

void SceneTransition::OnStart()
{
    SetEffectProgress();
    GetOwner()->SetNotDestroyBySceneEnd(true);
}

void SceneTransition::OnDestroy()
{
    s_changeScene = {};
    s_elapsedTime = 0.0f;
    s_duration = 0.0f;
    s_progress = 0.0f;
    s_phase = Phase::None;
    s_sceneLoadGenerationBeforeChange = 0;
    s_waitingForSceneLoad = false;
    s_skipNextDeltaTime = false;
}

void SceneTransition::Update()
{
    if (s_phase == Phase::None)
        return;

    if (s_phase == Phase::Opening)
    {
        if (s_waitingForSceneLoad)
        {
            if (Manager::GetSceneLoadGeneration() == s_sceneLoadGenerationBeforeChange)
                return;

            // シーン読込にかかった時間を、Openingの経過時間へ含めない。
            s_waitingForSceneLoad = false;
            s_skipNextDeltaTime = true;
            return;
        }

        if (s_skipNextDeltaTime)
        {
            s_skipNextDeltaTime = false;
            return;
        }
    }

    s_elapsedTime += Time::GetUnscaledDeltaTime();
    const float duration = std::max(s_duration, 0.0001f);
    const float rawProgress = std::clamp(s_elapsedTime / duration, 0.0f, 1.0f);

    if (s_phase == Phase::Closing)
    {
        s_progress = Easing::Evaluate(rawProgress, s_easeType);
        SetEffectProgress();

        if (rawProgress >= 1.0f)
        {
            s_progress = 1.0f;
            SetEffectProgress();
            s_phase = Phase::Opening;
            s_elapsedTime = 0.0f;
            if (s_changeScene)
                s_changeScene();
        }

        return;
    }

    const EaseType openingEaseType = Easing::Reverse(s_easeType);
    s_progress = 1.0f - Easing::Evaluate(rawProgress, openingEaseType);
    SetEffectProgress();

    if (rawProgress >= 1.0f)
    {
        s_progress = 0.0f;
        s_phase = Phase::None;
        s_changeScene = {};
        s_sceneLoadGenerationBeforeChange = 0;
        s_waitingForSceneLoad = false;
        s_skipNextDeltaTime = false;
        SetEffectProgress();
    }
}
