#include "pch.h"

#include "Scene/TitleScene.h"

#include "System/main.h"
#include "System/manager.h"
#include "Component/Transform.h"
#include "Component/SpriteRenderer.h"

#include "Script/TItleUI.h"
#include "Script/FieldRegistryTest.h"


void TitleScene::Init()
{
    // シーン初期化
    GameObject* titleBG = Manager::CreateEmpty(Vector3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f));
    titleBG->AddComponent<SpriteRenderer>(L"Assets/Local/Textures/TitleBG.png");
    titleBG->GetTransform()->SetScale(Vector3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f));

    const float LOGO_SCALE = 300.0f;
    GameObject* titleLogo = Manager::CreateEmpty(Vector3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 3.0f, 0.0f));
    titleLogo->AddComponent<SpriteRenderer>(L"Assets/Local/Textures/TitleLogo.png");
    titleLogo->GetTransform()->SetScale(Vector3(LOGO_SCALE * 3.0f, LOGO_SCALE * 2.0f, 1.0f)); // 3:2画像

    GameObject* titleUI =
        Manager::CreateEmpty(
            Vector3(SCREEN_WIDTH / 2.0f,
                    SCREEN_HEIGHT - SCREEN_HEIGHT / 4.0f,
                    0.0f));

    titleUI->AddComponent<SpriteRenderer>(L"Assets/Local/Textures/TitleUI.png");
    titleUI->AddComponent<TitleUI>();
    titleUI->GetTransform()->SetScale(Vector3(800.0f, 200.0f, 1.0f));

    GameObject* fieldRegistryTest = Manager::CreateEmpty("FieldRegistry Test");
    fieldRegistryTest->AddComponent<FieldRegistryTest>();
}
