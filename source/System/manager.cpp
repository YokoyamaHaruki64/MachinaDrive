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

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_curve_custom.h"

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

    // ImGuiの初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(GetWindow());
    ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetDeviceContext());
    ImGui::SetCurrentContext(ImGui::GetCurrentContext());
    ImGuiIO& io = ImGui::GetIO();

    // 入力
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Tab / 矢印 / Space / Enter
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // ゲームパッド
    io.ConfigWindowsMoveFromTitleBarOnly = true; // 中身をドラッグして誤移動しにくくする

    io.IniFilename = "config/imgui.ini"; // 設定ファイルの保存先を指定

    // スタイル
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 3.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 2.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8, 6);
    style.WindowPadding = ImVec2(10, 10);

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

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
static CurveColor testCurve;
void Manager::Update()
{
	Input::Update();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Debug Window");
    ImGui::Text("FPS: %.1f", 1.0f / ImGui::GetIO().DeltaTime);
    ImGui::Button("Button");
    ImGui::CurveColor("Curve Editor", testCurve);
    ImGui::End();
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

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	Renderer::End();

}
