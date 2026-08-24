#pragma once

// Precompiled header file
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS

// Windows APIとDirectX関連のヘッダーファイル
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "System/DirectXTex.h"

// C++標準ライブラリ
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <time.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <cassert>
#include <limits>

// 独自のヘッダーファイル
#include "math/MathCommon.h"
#include "System/GameObject.h"
#include "System/Component.h"
