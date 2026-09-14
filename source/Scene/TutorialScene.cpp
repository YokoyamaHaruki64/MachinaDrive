#include "pch.h"

#include "Scene/TutorialScene.h"

#include "System/Diagnostics/Logger.h"
#include "System/Core/manager.h"

void TutorialScene::Init()
{
    if (!Manager::LoadSceneObjects(GetSceneFilePath()))
    {
        Logger::Error("TutorialScene: Tutorial.sceneを読み込めません。");
    }
}
