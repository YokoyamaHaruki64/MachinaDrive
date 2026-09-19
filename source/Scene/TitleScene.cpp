#include "pch.h"

#include "Scene/TitleScene.h"

#include "System/Core/main.h"
#include "System/Core/manager.h"
#include "System/Core/input.h"
#include "Component/UIImage.h"
#include "Component/UICanvas.h"
#include "Component/UITransform.h"
#include "System/Rendering/renderer.h"

void TitleScene::Init()
{


    if (std::filesystem::exists(GetSceneFilePath()) && Manager::LoadSceneObjects(GetSceneFilePath()))
    {
        return;
    }



}
