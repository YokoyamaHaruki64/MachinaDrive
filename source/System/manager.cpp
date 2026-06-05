#include "System/manager.h"

#include "System/PrefabFactory.h"
#include "System/input.h"
#include "System/renderer.h"

#include "Component/Camera.h"
#include "Component/Primitive.h"

#include "Script/RotateCube.h"
#include "Script/CameraController.h"

std::vector<std::unique_ptr<GameObject>> Manager::m_GameObjects;
Camera* Manager::m_Camera = nullptr;

std::unordered_map<std::string,
    std::function<GameObject* (const Vector3&, const Vector3&, const Vector3&)>> Manager::m_FactoryMap;


void Manager::Init()
{
    RegisterFactory("Player", PlayerPrefabFactory);
    RegisterFactory("Enemy", EnemyPrefabFactory);


	Renderer::Init();
	Input::Init();

    GameObject* camera = CreateEmpty(Vector3(0.0f, 2.0f, -5.0f));
    camera->AddComponentForBeginScene<Camera>(Camera::MainCamera);
    camera->AddComponentForBeginScene<CameraController>();

    GameObject* field = CreateEmpty(Vector3::Zero());
    field->AddComponentForBeginScene<Plane>(
        L"Assets\\Textures\\glass2.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso");

    GameObject* obj = CreateEmpty(Vector3(0.0f, 1.0f, 0.0f));
    obj->AddComponentForBeginScene<Cube>(
        L"Assets\\Textures\\DarkMatReal.png",
        "BuildedShader\\Dice_VS.cso",
        "BuildedShader\\Dice_PS.cso");
    obj->AddComponentForBeginScene<RotateCube>();

    InstantiateForBeginScene(
        "Player",
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3::Zero(),
        Vector3::One());


	for (auto& gameObject : m_GameObjects)
	{
        gameObject->OnStart();
	}
}


void Manager::Uninit()
{
	Renderer::Uninit();
	Input::Uninit();
	for (auto& gameObject : m_GameObjects)
	{
        gameObject->OnDestroy();
	}
}

void Manager::Update()
{
	Input::Update();
	for (auto& gameObject : m_GameObjects)
	{
		gameObject->Update();
	}

    for (auto& gameObject : m_GameObjects)
    {
        gameObject->LateUpdate();
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

	for(auto& gameObject : m_GameObjects)
	{
		gameObject->Draw();
	}

	Renderer::End();

}

/*
GameObject obj = Allocate<GameObject>(new Camera());

Allocateの中身
事前確保された巨大メモリ
そこからオフセットやら何やらアルゴリズムでアドレス決定（初回はAllocateで確保した先頭アドレス）
そのアドレスの位置にCameraのコンストラクタを呼び出してCameraオブジェクトを構築
↑これがわからん
そのオブジェクトのアドレスを返す
*/
