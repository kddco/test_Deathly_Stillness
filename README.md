# Deathly Stillness Game 遊戲外掛專案

## 專案概述

這是一個針對 Unreal Engine 遊戲 "DeathlyStillnessGame" 開發的遊戲修改/外掛工具專案。專案使用 C++ 開發，包含 DLL 注入器和遊戲修改模組，主要提供 ESP（Extra Sensory Perception）透視功能。

## 專案結構

```
test_Deathly_Stillness/
├── test_Deathly_Stillness/    # 主執行檔專案（DLL 注入器）
│   └── injecter.cpp           # DLL 注入程式
├── Dll1/                      # 遊戲修改 DLL 專案
│   ├── dllmain.cpp           # DLL 主要邏輯
│   └── zombie_skeleton_renderer.h/cpp  # ESP 渲染模組
├── Dumper7/                   # Unreal Engine SDK 頭文件
│   └── SDK/                  # 遊戲類別定義和引擎綁定
├── OS-ImGui/                  # 自訂 ImGui DirectX 鉤子庫
└── imgui-1.92.2b/            # ImGui GUI 渲染庫
```

## 主要功能

### 1. DLL 注入器 (`test_Deathly_Stillness.exe`)
- 自動尋找遊戲視窗 "DeathlyStillnessGame"
- 獲取遊戲進程 ID 和基地址
- 使用 `CreateRemoteThread` + `LoadLibrary` 技術注入 DLL
- 支援動態記憶體分配和跨進程寫入

### 2. 遊戲修改 DLL (`Dll1.dll`)

#### ESP (透視) 功能
- **殭屍位置顯示**: 在螢幕上繪製紅色圓圈標記殭屍位置
- **3D 到 2D 投影**: 使用 `ProjectWorldLocationToScreen()` 將遊戲世界座標轉換為螢幕座標
- **即時更新**: 多執行緒監控殭屍狀態，自動移除已死亡的殭屍

#### 遊戲資訊獲取
- **玩家位置追蹤**: 即時獲取玩家 3D 座標 (X, Y, Z)
- **子彈計數**: 讀取武器彈藥數量（透過記憶體偏移 0x03CC）
- **Actor 枚舉**: 列出遊戲世界中所有 Actor 物件

#### 殭屍監控系統
- **存活狀態檢測**: 檢查殭屍血量 (偏移 0x04F0) 和死亡狀態 (偏移 0x04F9)
- **自動清理**: 每秒檢查並移除已死亡的殭屍
- **執行緒安全**: 使用 mutex 保護共享資料結構

## 技術特點

### 開發環境
- **IDE**: Visual Studio 2022
- **語言**: C++ (stdcpplatest)
- **平台**: Windows x64/x86
- **配置**: Release/Debug

### 核心技術
1. **Unreal Engine 逆向工程**
   - 使用 Dumper7 SDK 訪問遊戲物件
   - 透過 `UWorld::GetWorld()` 獲取遊戲世界
   - 使用 `PersistentLevel->Actors` 枚舉遊戲實體

2. **DirectX 渲染鉤子**
   - 使用 OS-ImGui 進行 DirectX 11 鉤子
   - ImGui 繪製 UI 元素和 ESP 標記
   - 支援自動檢測 DirectX 版本

3. **記憶體操作**
   - 跨進程記憶體讀寫
   - 直接記憶體偏移訪問遊戲資料
   - 動態記憶體分配和管理

4. **多執行緒設計**
   - 殭屍監控獨立執行緒
   - Mutex 互斥鎖保護共享資料
   - 非阻塞式狀態更新
## 使用方式

1. 啟動遊戲 "DeathlyStillnessGame"
2. 執行注入器 `test_Deathly_Stillness.exe`
3. 注入器會自動找到遊戲並注入 DLL
4. DLL 載入後會自動開啟控制台視窗
5. ESP 功能自動啟用，顯示殭屍位置

## 依賴函式庫

- **DirectX 11**: d3d11.lib, dxgi.lib, d3d9.lib
- **Windows API**: kernel32, user32, psapi
- **ImGui 1.92.2b**: GUI 渲染
- **OS-ImGui**: DirectX 鉤子框架
- **Dumper7 SDK**: Unreal Engine 遊戲綁定

## 重要說明

### 教育目的
本專案僅供學習和研究以下技術：
- Windows DLL 注入技術
- Unreal Engine 逆向工程
- DirectX 圖形鉤子
- 遊戲記憶體分析

### 免責聲明
- 此工具修改遊戲行為，可能違反遊戲服務條款
- 使用外掛可能導致帳號封禁
- 僅建議在單機環境或經授權的測試環境中使用
- 開發者不對任何不當使用承擔責任

## 貢獻指南

本專案主要用於技術研究和學習。如需貢獻，請確保：
1. 遵守法律和遊戲服務條款
2. 代碼僅用於教育目的
3. 不促進或鼓勵作弊行為
