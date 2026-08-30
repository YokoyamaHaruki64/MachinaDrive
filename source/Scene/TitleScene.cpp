#include "pch.h"

#include "Scene/TitleScene.h"

#include "System/Core/main.h"
#include "System/Core/manager.h"
#include "Component/UIImage.h"
#include "Component/UICanvas.h"
#include "Component/UITransform.h"
#include "System/Rendering/renderer.h"

#include "Script/TItleUI.h"
#include "Script/FieldRegistryTest.h"


void TitleScene::Init()
{
    if (std::filesystem::exists(GetSceneFilePath()) && Manager::LoadSceneObjects(GetSceneFilePath()))
    {
        return;
    }

    // シーン初期化
    GameObject* const canvas = Manager::CreateUICanvas("Title Canvas");

    GameObject* const titleBG = Manager::CreateUIObject("Title Background");
    titleBG->SetParent(canvas, ParentTransformMode::KeepLocal);
    UITransform* const backgroundTransform = titleBG->GetUITransform();
    titleBG->AddComponent<UIImage>()->SetTexturePath("Assets/Local/Textures/TitleBG.png");
    backgroundTransform->SetStretch(Vector2::One());

    GameObject* const titleLogo = Manager::CreateUIObject("Title Logo");
    titleLogo->SetParent(canvas, ParentTransformMode::KeepLocal);
    titleLogo->GetUITransform()->SetPos(Vector2(0.0f, -180.0f / 1080.0f));
    titleLogo->GetUITransform()->SetStretch(Vector2(900.0f / 1920.0f, 600.0f / 1080.0f));
    titleLogo->AddComponent<UIImage>()->SetTexturePath("Assets/Local/Textures/TitleLogo.png");

    GameObject* const titleUI = Manager::CreateUIObject("Title UI");
    titleUI->SetParent(canvas, ParentTransformMode::KeepLocal);
    titleUI->GetUITransform()->SetPos(Vector2(0.0f, 270.0f / 1080.0f));
    titleUI->GetUITransform()->SetStretch(Vector2(800.0f / 1920.0f, 200.0f / 1080.0f));
    titleUI->AddComponent<UIImage>()->SetTexturePath("Assets/Local/Textures/TitleUI.png");
    titleUI->AddComponent<TitleUI>();

    GameObject* fieldRegistryTest = Manager::CreateEmpty("FieldRegistry Test");
    fieldRegistryTest->AddComponent<FieldRegistryTest>();
}
