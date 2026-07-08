#include "pch.h"

#include "System/manager.h"

#include "System/PrefabFactory.h"
#include "System/input.h"
#include "System/renderer.h"
#include "Scene/Scene.h"
#include "Scene/TitleScene.h"

#include "Component/Camera.h"
#include "Component/Primitive.h"
#include "Component/SpriteRenderer.h"

#include "Script/RotateCube.h"
#include "Script/CameraController.h"

std::vector<std::unique_ptr<GameObject>> Manager::m_GameObjects;
std::vector<std::unique_ptr<GameObject>> Manager::m_pendingGameObjects; // オブジェクト追加用の一時的な格納場所

std::unique_ptr<Scene> Manager::m_currentScene;
std::unique_ptr<Scene> Manager::m_nextScene;

std::vector<RendererBase*> Manager::m_Renderers; // 描画用のレンダラーリスト

std::unordered_map<std::string,
    std::function<GameObject* (const Vector3&, const Vector3&, const Vector3&)>> Manager::m_FactoryMap;


void Manager::InitScene()
{
    if (m_currentScene)
    {
        m_currentScene->Uninit();

        // ゲームオブジェクトの破棄処理
        PendingGameObjectsProcess();
        for (auto& gameObject : m_GameObjects)
        {
            gameObject->OnDestroy();
        }
        m_GameObjects.clear();

        m_currentScene.reset();
    }

    if (m_nextScene)
    {
        m_currentScene = std::move(m_nextScene);
        m_currentScene->Init();
    }

    PendingGameObjectsProcess();

    for (auto& gameObject : m_GameObjects)
    {
        gameObject->OnStart();
    }

}

void Manager::Init()
{
    RegisterFactory("Player", PlayerPrefabFactory);
    RegisterFactory("Enemy", EnemyPrefabFactory);
    RegisterFactory("Bullet", BulletPrefabFactory);
    RegisterFactory("Tree", TreePrefabFactory);
    RegisterFactory("ExplosionEffect", ExplosionPrefabFactory);
    RegisterFactory("Ball", BoundBallPrefabFactory);
    RegisterFactory("Ripple", RipplePrefabFactory);
    RegisterFactory("Firework", FireworksEffectPrefabFactory);
    RegisterFactory("ImpactBurst", ImpactBurstEffectPrefabFactory);

	Renderer::Init();
	Input::Init();

    m_nextScene = std::make_unique<TitleScene>();
    InitScene();
}


void Manager::Uninit()
{
	Renderer::Uninit();
	Input::Uninit();
    PendingGameObjectsProcess();
	for (auto& gameObject : m_GameObjects)
	{
        gameObject->OnDestroy();
	}
    m_GameObjects.clear();
}

void Manager::Update()
{
	Input::Update();

    PendingGameObjectsProcess();

    if (m_nextScene)
    {
        InitScene();
    }


	for (auto& gameObject : m_GameObjects)
	{
		gameObject->Update();
	}

    for (auto& gameObject : m_GameObjects)
    {
        gameObject->LateUpdate();
    }

    // 破棄されたオブジェクトの削除
    for (size_t i = 0; i < m_GameObjects.size();)
    {
        if (m_GameObjects[i]->IsDestroyed())
        {
            m_GameObjects[i]->OnDestroy();

            // 末尾と入れ替え
            std::swap(m_GameObjects[i], m_GameObjects.back());

            // 末尾削除
            m_GameObjects.pop_back();

            // インクリメントしない
            // swap後に新しい要素が i に来るので再チェック
        }
        else
        {
            ++i;
        }
    }

}

void Manager::Draw()
{
	Renderer::Begin();

    if (Camera* camera = Camera::GetMainCamera())
    {
        camera->SetViewProjection();
    }

	LIGHT light{};
	Vector3 lightDirection = Vector3(1.0f, -1.0f, 1.0f).Normalized();

	light.Enable = TRUE;
	light.Direction = XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
	light.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	Renderer::SetLight(light);	

    for( auto& renderer : m_Renderers)
    {
        renderer->Submit();
    }
	//for(auto& gameObject : m_GameObjects)
	//{
	//	gameObject->Draw();
	//}

	Renderer::Draw();
	Renderer::End();

}
