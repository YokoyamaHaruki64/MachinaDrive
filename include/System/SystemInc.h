#pragma once

// Script / Component実装で頻出するSystem層の依存をまとめる。
// 個別ヘッダーは自己完結を保つため、このファイルに依存させない。
#include "System/Component.h"
#include "System/ComponentRegistry.h"
#include "System/FileSystem.h"
#include "System/GameObject.h"
#include "System/GM_Time.h"
#include "System/input.h"
#include "System/Logger.h"
#include "System/main.h"
#include "System/manager.h"
#include "System/YAMLDocumentIO.h"
#include "System/YAMLReflection.h"

#include "Component/Transform.h"
