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


constexpr wchar_t CLASS_NAME[] = L"AppClass";
#if GM_ENABLE_EDITOR
constexpr wchar_t WINDOW_NAME[] = L"GameMaker Editor";
#else
constexpr wchar_t WINDOW_NAME[] = L"MachinaDrive";
#endif


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#if GM_ENABLE_EDITOR
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif


HWND g_Window;

HWND GetWindow()
{
	return g_Window;
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{


	WNDCLASSEXW wcex;
	{
		wcex.cbSize = sizeof(WNDCLASSEXW);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassExW(&wcex);


		RECT rc{};
#if GM_ENABLE_EDITOR
		rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
		const DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_MAXIMIZE;
		AdjustWindowRect(&rc, windowStyle, FALSE);

		g_Window = CreateWindowExW(0, CLASS_NAME, WINDOW_NAME, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
#else
		// Runtimeはモニター全域をクライアント領域にしたボーダーレスで起動する。
		const POINT primaryMonitorPoint{ 0, 0 };
		const HMONITOR primaryMonitor = MonitorFromPoint(
			primaryMonitorPoint,
			MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO monitorInfo{ sizeof(monitorInfo) };
		if (primaryMonitor && GetMonitorInfo(primaryMonitor, &monitorInfo))
		{
			rc = monitorInfo.rcMonitor;
		}
		else
		{
			rc.right = GetSystemMetrics(SM_CXSCREEN);
			rc.bottom = GetSystemMetrics(SM_CYSCREEN);
		}

		const DWORD windowStyle = WS_POPUP;
		g_Window = CreateWindowExW(
			WS_EX_APPWINDOW,
			CLASS_NAME,
			WINDOW_NAME,
			windowStyle,
			rc.left,
			rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr,
			nullptr,
			hInstance,
			nullptr);
#endif
	}

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	// 初期化が終わるまでShowWindowを呼ばず、OSの未描画領域を見せない。

	LogSystem::Init();
	if (!Manager::Init())
	{
        LogSystem::Shutdown();
        CoUninitialize();
        UnregisterClassW(CLASS_NAME, wcex.hInstance);
		return EXIT_FAILURE;
	}

	// 描画済みのフレームからEditorは最大化、Runtimeはボーダーレスで表示する。
#if GM_ENABLE_EDITOR
	ShowWindow(g_Window, SW_SHOWMAXIMIZED);
#else
	ShowWindow(g_Window, SW_SHOW);
#endif
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

	UnregisterClassW(CLASS_NAME, wcex.hInstance);

	Manager::Uninit();
    LogSystem::Shutdown();

	CoUninitialize();

	return (int)msg.wParam;
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INPUT)
    {
        InputInternal::ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_ACTIVATEAPP)
    {
#if GM_ENABLE_EDITOR
        // Editorではアプリ全体の復帰とGameWindowのフォーカスを分離する。
        if (!wParam)
        {
            InputInternal::SetWindowCursorState(false);
        }
#else
        // Runtimeではアプリ復帰時にゲーム側のカーソル要求を再適用する。
        InputInternal::SetWindowCursorState(wParam != FALSE);
#endif

        return 0;
    }

#if GM_ENABLE_EDITOR
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

