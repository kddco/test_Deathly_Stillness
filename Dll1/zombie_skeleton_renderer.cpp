#include "zombie_skeleton_renderer.h"
#include <iostream>

// ImGui includes for rendering
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

// 檢查指針有效性
bool IsValidPointer(void* ptr) {
    if (!ptr) return false;
    __try {
        volatile auto test = *static_cast<uintptr_t*>(ptr);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void RenderZombieDots() {
    static bool imguiStatusReported = false;
    static bool lastImguiStatus = false;
    static bool imguiInitAttempted = false;
    
    // 嘗試初始化ImGui context
    if (!imguiInitAttempted) {
        if (ImGui::GetCurrentContext() == nullptr) {
            std::cout << "[INFO] No existing ImGui context found" << std::endl;
            std::cout << "[INFO] Attempting to create ImGui context..." << std::endl;
            
            try {
                ImGuiContext* ctx = ImGui::CreateContext();
                if (ctx != nullptr) {
                    ImGui::SetCurrentContext(ctx);
                    
                    // 基本設置
                    ImGuiIO& io = ImGui::GetIO();
                    io.DisplaySize = ImVec2(1920, 1080); // 假設螢幕解析度
                    
                    // 建立字體圖集
                    unsigned char* pixels;
                    int width, height;
                    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                    
                    // 設置基本的渲染器標誌
                    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
                    
                    std::cout << "[INFO] ImGui context created and configured!" << std::endl;
                } else {
                    std::cout << "[ERROR] Failed to create ImGui context" << std::endl;
                }
            } catch (...) {
                std::cout << "[ERROR] Exception while creating ImGui context" << std::endl;
            }
        } else {
            std::cout << "[INFO] Existing ImGui context found!" << std::endl;
        }
        imguiInitAttempted = true;
    }
    
    try {
        SDK::UWorld* World = SDK::UWorld::GetWorld();
        if (!World || !IsValidPointer(World) || !World->PersistentLevel) {
            return;
        }

        SDK::APlayerController* PC = SDK::UGameplayStatics::GetPlayerController(World, 0);
        if (!PC || !IsValidPointer(PC)) {
            return;
        }

        auto& Actors = World->PersistentLevel->Actors;
        int zombieCount = 0;
        bool imguiAvailable = (ImGui::GetCurrentContext() != nullptr);
        
        // 如果ImGui可用，調用NewFrame
        if (imguiAvailable) {
            try {
                ImGuiIO& io = ImGui::GetIO();
                io.DeltaTime = 0.016f; // 假設60FPS
                
                // 確保字體圖集已建立
                if (!io.Fonts->IsBuilt()) {
                    unsigned char* pixels;
                    int width, height;
                    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                }
                
                ImGui::NewFrame();
            } catch (...) {
                // 如果NewFrame失敗，標記為不可用
                imguiAvailable = false;
            }
        }
        
        // 只在ImGui狀態改變時報告
        if (!imguiStatusReported || (imguiAvailable != lastImguiStatus)) {
            if (imguiAvailable) {
                std::cout << "[INFO] ImGui context is now available! Starting ESP rendering..." << std::endl;
            } else {
                std::cout << "[INFO] ImGui context still not available, waiting..." << std::endl;
            }
            lastImguiStatus = imguiAvailable;
            imguiStatusReported = true;
        }
        
        for (int32_t i = 0; i < Actors.Num(); i++) {
            SDK::AActor* Actor = Actors[i];
            
            if (!Actor || !IsValidPointer(Actor)) {
                continue;
            }

            if (IsZombieActor(Actor)) {
                zombieCount++;
                
                try {
                    SDK::FVector location = Actor->K2_GetActorLocation();
                    SDK::FVector2D screenLocation;
                    
                    if (PC->ProjectWorldLocationToScreen(location, &screenLocation, false)) {
                        if (imguiAvailable) {
                            try {
                                ImDrawList* drawList = ImGui::GetForegroundDrawList();
                                if (drawList != nullptr) {




                                    drawList->AddCircleFilled(
                                        ImVec2(screenLocation.X, screenLocation.Y),
                                        4.0f,
                                        IM_COL32(255, 0, 0, 255)
                                    );
                                }
                            } catch (...) {
                                // 靜默處理ImGui錯誤
                            }
                        }
                    }
                } catch (...) {
                    continue;
                }
            }
        }
        
        // 結束ImGui frame
        if (imguiAvailable) {
            try {
                ImGui::EndFrame();
                ImGui::Render();
            } catch (...) {
                // 靜默處理ImGui錯誤
            }
        }
        
        // 只在找到zombie數量改變時報告
        static int lastZombieCount = -1;
        if (zombieCount != lastZombieCount) {
            std::cout << "[INFO] Found " << zombieCount << " zombies, ImGui status: " 
                      << (imguiAvailable ? "Available" : "Not Available") << std::endl;
            lastZombieCount = zombieCount;
        }
    }
    catch (...) {
        // 靜默處理異常
    }
}

// 檢查Actor是否為Zombie
bool IsZombieActor(SDK::AActor* actor) {
    if (!actor || !IsValidPointer(actor)) {
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

