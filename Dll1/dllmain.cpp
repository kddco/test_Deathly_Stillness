#include "windows.h"
#include <stdint.h>
#include <iostream>
#include "SDK.hpp"
#include "SDK/Engine_classes.hpp"
//#include "zombie_skeleton_renderer.h"
#include <d3d11.h>
#include <tchar.h>
#include <dxgi.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <psapi.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#include "OS-ImGui.h"

//global
HMODULE GlobalhModule;
SDK::FVector2D GlobalCurrentActorscreenLocation;
std::vector<SDK::AActor*> ZombieActors;
std::mutex ZombieActorsMutex;
std::vector<SDK::FVector2D> ZombieScreenPositions;
std::mutex ZombieScreenPositionsMutex;
void DrawCallBack()
{
    ImGui::Begin("Menu");
    {
        std::lock_guard<std::mutex> lock(ZombieScreenPositionsMutex);

        // 繪製所有殭屍的ESP標記
        for (const auto& position : ZombieScreenPositions) {
            Gui.Circle(ImVec2(position.X, position.Y), 6.0f, IM_COL32(255, 0, 0, 255), 0, 2.0f);
        }

    }ImGui::End();

}
void GetbulletCount() {
    /* Functions returning "static" instances */
    SDK::UEngine* Engine = SDK::UEngine::GetEngine();
    SDK::UWorld* World = SDK::UWorld::GetWorld();
    SDK::AActor* FoundActor = SDK::UGameplayStatics::GetActorOfClass(
        World,
        SDK::AWeapon_Master_C::StaticClass()
    );

    SDK::AWeapon_Master_C* WeaponMaster = nullptr;
    if (FoundActor) {
        WeaponMaster = static_cast<SDK::AWeapon_Master_C*>(FoundActor);
    }

    if (WeaponMaster) {

        // 成功獲取子彈數量
        while (1) {
            int32_t bulletCount = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(WeaponMaster) + 0x03CC);
            std::cout << "Current bullet count: " << bulletCount << std::endl;
        }
    }
    else {
        // 處理未找到武器的情況
    }
}
void GetPlayerPosition() {
    SDK::APlayerController* PC = SDK::UGameplayStatics::GetPlayerController(SDK::UWorld::GetWorld(), 0);
    if (PC)
    {
        SDK::APawn* PlayerPawn = PC->K2_GetPawn();
        if (PlayerPawn)
        {
           
            while (1) {
                SDK::FVector Location = PlayerPawn->K2_GetActorLocation();
                std::cout << Location.X << "," << Location.Y << "," << Location.Z << std::endl;
            }
        }
    }
}

void EnumerateAllActors() {
    SDK::UWorld* World = SDK::UWorld::GetWorld();
    if (!World || !World->PersistentLevel) {
        std::cout << "World or PersistentLevel is null!" << std::endl;
        return;
    }

    std::cout << "=== Enumerating all Actors in PersistentLevel ===" << std::endl;
    
    // 獲取 PersistentLevel 中的所有 Actor
    auto& Actors = World->PersistentLevel->Actors;
    
    std::cout << "Total Actors count: " << Actors.Num() << std::endl;
    
    for (int32_t i = 0; i < Actors.Num(); i++) {
        SDK::AActor* Actor = Actors[i];
        
        if (!Actor) {
            continue; // 跳過空的 Actor
        }
        
        // 輸出 Actor 的基本信息
        std::cout << "[" << i << "] " << Actor->GetFullName();
        
        // 獲取 Actor 位置
        SDK::FVector Location = Actor->K2_GetActorLocation();
        std::cout << " | Position: (" << Location.X << ", " << Location.Y << ", " << Location.Z << ")";
        
        // 檢查是否為特定類型的 Actor
        if (Actor->IsA(SDK::APawn::StaticClass())) {
            std::cout << " | Type: Pawn";
        } else if (Actor->IsA(SDK::APlayerController::StaticClass())) {
            std::cout << " | Type: PlayerController";
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "=== Enumeration complete ===" << std::endl;
}
// 檢查Actor是否為Zombie
bool CheckIsZombieActor(SDK::AActor* actor) {
    if (!actor) {
        return false;
    }

    try {
        std::string actorName = actor->GetFullName();
        return actorName.find("Zombie_BP") != std::string::npos;
    }
    catch (...) {
        return false;
    }
}

// 檢查殭屍是否還活著（血量是否為正數）
bool IsZombieAlive(SDK::AActor* actor) {
    // 1. 檢查基礎 Actor 狀態
    if (!actor || actor->bActorIsBeingDestroyed) {
        return false;
    }

    try {
        // 2. 使用記憶體偏移直接讀取血量和死亡狀態
        // Health 偏移: 0x04F0, Dead 偏移: 0x04F9
        uintptr_t actorAddress = reinterpret_cast<uintptr_t>(actor);

        // 讀取 Health (float, 偏移 0x04F0)
        float health = *reinterpret_cast<float*>(actorAddress + 0x04F0);

        // 讀取 Dead (bool, 偏移 0x04F9)
        bool isDead = *reinterpret_cast<bool*>(actorAddress + 0x04F9);

        // 檢查死亡狀態
        if (isDead) {
            return false;
        }

        // 檢查血量
        if (health <= 0.0f) {
            return false;
        }

        return true;
    }
    catch (...) {
        // 發生異常時認為殭屍已死亡
        return false;
    }
}

struct InitData {
    SDK::APlayerController* PC;
    SDK::UWorld* World;
    SDK::TArray<SDK::AActor*> Actors;
};

InitData InitBaseAddress() {
    InitData result = { nullptr, nullptr, SDK::TArray<SDK::AActor*>() };

    SDK::UWorld* World = SDK::UWorld::GetWorld();
    if (!World || !World->PersistentLevel) {
        std::cout << "World or PersistentLevel is null!" << std::endl;
        return result;
    }

    //玩家控制器物件
    SDK::APlayerController* PC = nullptr;
    if (!World->OwningGameInstance) {
        std::cout << "OwningGameInstance is null!" << std::endl;
        return result;
    }
    // 确保 LocalPlayers 数组不为空
    if (World->OwningGameInstance->LocalPlayers.Num() == 0) {
        std::cout << "No LocalPlayers found!" << std::endl;
        return result;
    }
    // 获取第一个本地玩家的 PlayerController
    SDK::ULocalPlayer* LocalPlayer = World->OwningGameInstance->LocalPlayers[0];
    if (!LocalPlayer) {
        std::cout << "LocalPlayer is null!" << std::endl;
        return result;
    }
    PC = LocalPlayer->PlayerController;
    if (!PC) {
        std::cout << "無法取得APlayerController" << std::endl;
        return result;
    }

    std::cout << "=== Enumerating all Actors in PersistentLevel ===" << std::endl;
    // 獲取 PersistentLevel 中的所有 Actor
    auto& Actors = World->PersistentLevel->Actors;

    result.PC = PC;
    result.World = World;
    result.Actors = Actors;
    return result;
}

// 殭屍監控線程函數
void ZombieMonitorThread() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(ZombieActorsMutex);

            // 檢查並移除已死亡的殭屍
            auto it = std::remove_if(ZombieActors.begin(), ZombieActors.end(),
                [](SDK::AActor* actor) {
                    if (!actor) {
                        return true; // 移除空指標
                    }

                    // 檢查殭屍是否還活著
                    if (!IsZombieAlive(actor)) {
                        std::cout << "殭屍已死亡，從列表中移除: " << actor->GetName() << std::endl;
                        return true; // 移除已死亡的殭屍
                    }

                    return false; // 保留活著的殭屍
                });

            ZombieActors.erase(it, ZombieActors.end());
        }

        // 每1000毫秒檢查一次
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void GetZombieActors(const InitData& initData) {
    auto& Actors = initData.Actors;

    std::lock_guard<std::mutex> lock(ZombieActorsMutex);

    // 清空之前的殭屍列表
    ZombieActors.clear();

    for (int32_t i = 0; i < Actors.Num(); i++) {
        SDK::AActor* CurrentActor = Actors[i];
        if (!CurrentActor) {
            continue;
        }

        // 檢查是否為殭屍
        if (!CheckIsZombieActor(CurrentActor)) {
            continue;
        }

        // 檢查殭屍是否還活著
        if (!IsZombieAlive(CurrentActor)) {
            continue;
        }

        // 將活著的殭屍加入列表
        ZombieActors.push_back(CurrentActor);
    }
}

void EzZombieESP(const InitData& initData) {
    SDK::APlayerController* PC = initData.PC;

    std::vector<SDK::AActor*> localZombieActors;
    {
        std::lock_guard<std::mutex> lock(ZombieActorsMutex);
        localZombieActors = ZombieActors; // 複製一份以避免長時間鎖定
    }

    std::vector<SDK::FVector2D> validScreenPositions;

    // 收集所有有效的殭屍螢幕座標
    for (SDK::AActor* CurrentActor : localZombieActors) {
        if (!CurrentActor) {
            continue;
        }

        std::cout << "當前處理的Actor" << CurrentActor->GetName() << std::endl;
        SDK::FVector CurrentActorlocation;
        try {
            CurrentActorlocation = CurrentActor->K2_GetActorLocation();
            std::cout << "x軸: " << CurrentActorlocation.X << "Y軸: " << CurrentActorlocation.Y << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
            std::cout << "處理Actor座標錯誤" << std::endl;
            continue;
        }

        // 檢查位置是否有效
        if (std::isnan(CurrentActorlocation.X) || std::isnan(CurrentActorlocation.Y) || std::isnan(CurrentActorlocation.Z)) {
            std::cout << CurrentActor->GetName() << " 位置數據無效" << std::endl;
            continue;
        }

        SDK::FVector2D CurrentActorscreenLocation;
        bool bProjectSuccess = PC->ProjectWorldLocationToScreen(CurrentActorlocation, &CurrentActorscreenLocation, false);

        if (!bProjectSuccess) {
            std::cout << "3D轉2D 投影失敗" << std::endl;
            continue; // 投影失敗，跳過
        }

        // 檢查螢幕座標是否在合理範圍內
        if (CurrentActorscreenLocation.X < -1000 || CurrentActorscreenLocation.X > 10000 ||
            CurrentActorscreenLocation.Y < -1000 || CurrentActorscreenLocation.Y > 10000) {
            std::cout << "座標超出合理範圍" << std::endl;
            continue; // 座標超出合理範圍
        }

        // 收集有效的螢幕座標
        validScreenPositions.push_back(CurrentActorscreenLocation);
    }

    // 更新全局螢幕座標列表
    {
        std::lock_guard<std::mutex> lock(ZombieScreenPositionsMutex);
        ZombieScreenPositions = validScreenPositions;
    }

    // 一次性繪製所有殭屍ESP
    if (!validScreenPositions.empty()) {
        Gui.Start(GlobalhModule, DrawCallBack, OSImGui::DirectXType::AUTO);
    }
}
void dohack() {
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);
    //GetbulletCount();
    // 測試 console 是否正常工作
    std::cout << "Console initialized successfully!" << std::endl;
    std::cout << "Starting position tracking..." << std::endl;
    //EnumerateAllActors();

    InitData initData = InitBaseAddress();
    if (!initData.PC) {
        std::cout << "Failed to initialize base address!" << std::endl;
        return;
    }

    GetZombieActors(initData);
    // 創建殭屍監控線程
    std::thread zombieMonitorThread(ZombieMonitorThread);
    zombieMonitorThread.detach(); // 設為分離線程
    std::cout << "殭屍監控線程已啟動" << std::endl;

    while (1) {
        
        EzZombieESP(initData);
    }

}
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        GlobalhModule = hModule;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)dohack, hModule, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

