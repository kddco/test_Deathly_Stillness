#pragma once

#include "windows.h"
#include <stdint.h>
#include <vector>
#include <string>
#include "SDK.hpp"
#include "SDK/Engine_classes.hpp"

// 簡化ESP系統 - 只顯示點
void RenderZombieDots();
bool IsValidPointer(void* ptr);

// 檢查Actor是否為Zombie
bool IsZombieActor(SDK::AActor* actor);