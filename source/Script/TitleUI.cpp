#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/TItleUI.h"

#include "Scene/GameScene.h"
#include "Component/UITransform.h"
#include "Script/SceneTransition.h"


void TitleUI::OnStart()
{
    m_elapsedTime = 0.0f;
    m_originalOffset = m_owner->GetUITransform()->GetOffset();
}

void TitleUI::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    // サイン波の計算
    float offsetY = std::sin(m_elapsedTime * Math::PI_2 * m_frequency) * m_amplitude;
    // 元の位置にオフセットを加える
    m_owner->GetUITransform()->SetOffset(m_originalOffset + Vector2(0.0f, offsetY));

    if (Input::GetKeyTrigger(Input::KeyCode::ENTER))
        SceneTransition::ChangeScene<GameScene>(0.8f, EaseType::InOutQuad);

}
