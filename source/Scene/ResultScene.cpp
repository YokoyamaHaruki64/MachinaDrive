#include "pch.h"

#include "Scene/ResultScene.h"

#include "System/main.h"
#include "System/manager.h"
#include "Component/Transform.h"
#include "Component/SpriteRenderer.h"

#include "Script/ResultUI.h"


void ResultScene::Init()
{
    // シーン初期化
    GameObject* titleBG = Manager::CreateEmpty(Vector3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f));
    titleBG->AddComponent<SpriteRenderer>(L"Assets/Textures/ResultBG.png");
    titleBG->GetTransform()->SetScale(Vector3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f));

    const float LOGO_SCALE = 300.0f;
    GameObject* logo = Manager::CreateEmpty(Vector3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 3.0f, 0.0f));
    logo->AddComponent<SpriteRenderer>(L"Assets/Textures/TitleLogo.png");
    logo->GetTransform()->SetScale(Vector3(LOGO_SCALE * 3.0f, LOGO_SCALE * 2.0f, 1.0f)); // 3:2画像

    GameObject* resultUI =
        Manager::CreateEmpty(
            Vector3(SCREEN_WIDTH / 2.0f,
                    SCREEN_HEIGHT - SCREEN_HEIGHT / 4.0f,
                    0.0f));

    resultUI->AddComponent<SpriteRenderer>(L"Assets/Textures/ResultUI.png");
    resultUI->AddComponent<ResultUI>();
    resultUI->GetTransform()->SetScale(Vector3(800.0f, 200.0f, 1.0f));
}
