#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;


#include <DirectXMath.h>
using namespace DirectX;

#include "DirectXTex.h"
#if _DEBUG
#pragma comment (lib, "Library/DirectXTex_Debug.lib")
#else
#pragma comment (lib, "Library/DirectXTex_Release.lib")
#endif





#pragma comment (lib, "winmm.lib")


#define SCREEN_WIDTH	(1600)
#define SCREEN_HEIGHT	(900)


HWND GetWindow();

void Invoke(std::function<void()> Function, int Time);

