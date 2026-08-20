#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/TItleUI.h"

#include "Scene/GameScene.h"


void TitleUI::OnStart()
{
    m_elapsedTime = 0.0f;
    m_originalPosition = transform()->GetPosition();
}

void TitleUI::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    // サイン波の計算
    float offsetY = std::sin(m_elapsedTime * Math::PI_2 * m_frequency) * m_amplitude;
    // 元の位置にオフセットを加える
    Vector3 newPosition = m_originalPosition + Vector3(0.0f, offsetY, 0.0f);
    transform()->SetPosition(newPosition);

    if (Input::GetKeyTrigger(Input::KK_ENTER))
        Manager::ChangeScene<GameScene>();

}
