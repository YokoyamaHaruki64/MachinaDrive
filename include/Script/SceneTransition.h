#pragma once

#include "System/Core/Script.h"
#include "System/Core/manager.h"

#include <cstdint>
#include <functional>

class SceneTransition final : public Script
{
    REGISTER_COMPONENT_OPTIONS(SceneTransition, "Scene Transition", false, true);

    enum class Phase
    {
        None,
        Closing,
        Opening,
    };

    inline static std::function<void()> s_changeScene;
    inline static float s_elapsedTime{};
    inline static float s_duration{};
    inline static float s_progress{};
    inline static EaseType s_easeType{ EaseType::InOutQuad };
    inline static Phase s_phase{ Phase::None };
    inline static uint64_t s_sceneLoadGenerationBeforeChange{};
    inline static bool s_waitingForSceneLoad = false;
    inline static bool s_skipNextDeltaTime = false;

    static void SetEffectProgress();

public:
    SceneTransition() = delete;
    explicit SceneTransition(GameObject* owner)
        : Script(owner)
    {
        owner->SetNotDestroyBySceneEnd(true);
    }

    template <typename T>
    static bool ChangeScene(
        float duration,
        EaseType easeType = EaseType::InOutQuad)
    {
        if (s_phase != Phase::None)
            return false;

        if (duration <= 0.0f)
        {
            Manager::ChangeScene<T>();
            return true;
        }

        s_changeScene = []
        {
            Manager::ChangeScene<T>();
        };
        s_elapsedTime = 0.0f;
        s_duration = duration;
        s_progress = 0.0f;
        s_easeType = easeType;
        s_phase = Phase::Closing;
        s_sceneLoadGenerationBeforeChange = Manager::GetSceneLoadGeneration();
        s_waitingForSceneLoad = true;
        s_skipNextDeltaTime = false;
        SetEffectProgress();
        return true;
    }

    static bool IsTransitionEffect()
    {
        return s_phase != Phase::None;
    }

    static float GetProgress()
    {
        return s_progress;
    }

    void OnStart() override;
    void OnDestroy() override;
    void Update() override;
};
