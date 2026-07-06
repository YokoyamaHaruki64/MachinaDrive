#include "pch.h"

#include "Script/ResultUI.h"

#include "System/GM_Time.h"
#include "System/input.h"
#include "System/manager.h"
#include "Scene/TitleScene.h"

#include "Component/Transform.h"

void ResultUI::OnStart()
{
    m_elapsedTime = 0.0f;
    m_originalPosition = transform()->GetPosition();
}

void ResultUI::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    // サイン波の計算
    float offsetY = std::sin(m_elapsedTime * Math::PI_2 * m_frequency) * m_amplitude;
    // 元の位置にオフセットを加える
    Vector3 newPosition = m_originalPosition + Vector3(0.0f, offsetY, 0.0f);
    transform()->SetPosition(newPosition);

    if (Input::GetKeyTrigger(Input::KK_ENTER))
        Manager::ChangeScene<TitleScene>();

}
