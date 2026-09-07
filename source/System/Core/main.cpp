#include "pch.h"

#include "System/Core/main.h"
#include "System/Core/manager.h"
#include "System/Core/input.h"
#include "System/Core/GM_Time.h"
#include "System/Diagnostics/LogSystem.h"
#include "System/Diagnostics/PerformanceMonitor.h"
#include "System/Rendering/renderer.h"
#if GM_ENABLE_EDITOR
#include "System/Editor/EditorSystem.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui.h"
#endif
#include <thread>


const char* CLASS_NAME = "AppClass";
const char* WINDOW_NAME = "DX11ゲーム";


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#if GM_ENABLE_EDITOR
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif


HWND g_Window;

HWND GetWindow()
{
	return g_Window;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{


	WNDCLASSEX wcex;
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassEx(&wcex);


		RECT rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
		const DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_MAXIMIZE;
		AdjustWindowRect(&rc, windowStyle, FALSE);

		g_Window = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	}

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	// WS_MAXIMIZE により、表示せずに最大化状態のクライアント領域を確定させる。
	// 初期化が終わるまで ShowWindow を呼ばず、OS の未描画領域を見せない。

	LogSystem::Init();
	if (!Manager::Init())
	{
        LogSystem::Shutdown();
        CoUninitialize();
        UnregisterClass(CLASS_NAME, wcex.hInstance);
		return EXIT_FAILURE;
	}

	// 描画済みのフレームから最大化状態で表示する。
	ShowWindow(g_Window, SW_SHOWMAXIMIZED);
	UpdateWindow(g_Window);

	DWORD dwExecLastTime;
	DWORD dwCurrentTime;
	timeBeginPeriod(1);
	dwExecLastTime = timeGetTime();
	dwCurrentTime = 0;



	MSG msg;
	while(1)
	{
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }
		else
		{
			dwCurrentTime = timeGetTime();
            DWORD deltaTime = dwCurrentTime - dwExecLastTime;
			if( deltaTime >= (1000 / 60))
			{
				dwExecLastTime = dwCurrentTime;
				Time::Update(deltaTime / 1000.0f);
				PerformanceMonitor::RecordFrameTime(Time::GetUnscaledDeltaTime());
				Manager::Update();
				Manager::Draw();
                LogSystem::Flush();
			}
		}
	}

	timeEndPeriod(1);

	UnregisterClass(CLASS_NAME, wcex.hInstance);

	Manager::Uninit();
    LogSystem::Shutdown();

	CoUninitialize();

	return (int)msg.wParam;
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if GM_ENABLE_EDITOR
    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE && Manager::IsPlayMode())
    {
        // Play中のEscはアプリケーション終了ではなく、カーソル制限の解除に使う。
        Input::SetMouseLocked(false);
        return 0;
    }

    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;
#endif

	switch(uMsg)
	{
	case WM_CLOSE:
#if GM_ENABLE_EDITOR
        if (hWnd == GetWindow() && EditorSystem::RequestApplicationClose())
        {
            return 0;
        }
#endif
        DestroyWindow(hWnd);
        return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
			Renderer::ResizeBackBuffer(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_KEYDOWN:
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

