#pragma once

// Script / Component実装で頻出するSystem層の依存をまとめる。
// 個別ヘッダーは自己完結を保つため、このファイルに依存させない。
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Editor/FileSystem.h"
#include "System/Core/GameObject.h"
#include "System/Core/GM_Time.h"
#include "System/Core/input.h"
#include "System/Diagnostics/Logger.h"
#include "System/Core/main.h"
#include "System/Core/manager.h"
#include "System/Serialization/YAMLDocumentIO.h"
#include "System/Serialization/YAMLReflection.h"

#include "Component/Transform.h"
