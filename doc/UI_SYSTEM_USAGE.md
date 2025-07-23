# UI 系統使用指南

## 概述

新的 IUISystem 系統提供了分層 UI 架構，解決了原來 UI 創建硬編碼在 EngineContext 中的問題，並支援跨場景的 UI 持續性。

## 核心概念

### 1. UI 分層架構

UI 系統採用分層設計，每一層有不同的用途和優先權：

```cpp
enum class UILayerType {
    Background,    // 背景層 (priority: 0)
    World,         // 世界/遊戲層 (priority: 100)
    Interface,     // 介面層 (priority: 200)
    Overlay,       // 覆蓋層 - 選單、對話框 (priority: 300)
    Persistent,    // 持續層 - HUD、除錯資訊 (priority: 400)
    System         // 系統層 - 最上層 (priority: 500)
};
```

### 2. 跨場景持續性

- **場景特定層級**: 隨場景切換自動清理
- **持續層級**: 跨場景保持，適合 HUD、除錯資訊等

### 3. 事件驅動

- **組件級回調**: 每個組件可以有自己的事件處理
- **全域事件**: 系統級的事件處理
- **類型安全**: 強類型的事件系統

## 解決的問題

### 🚫 原來的問題

```cpp
// EngineContext.cpp - 硬編碼 UI 創建
STDMETHODIMP EngineContext::LoadAssets(...) {
    // 添加新UI系統測試內容
    if (uiManager_) {
        auto* bgImage = uiManager_->CreateImage(L"bg.bmp", 50, 50, 200, 150, true);
        auto* button1 = uiManager_->CreateButton(L"按鈕1", 10, 10, 80, 30, 
            []() { MessageBoxA(nullptr, "按鈕1被點擊了!", "UI測試", MB_OK); });
    }
}
```

**問題**:
1. UI 創建邏輯散布在引擎核心中
2. 無法跨場景重用 UI
3. 事件處理寫死在程式碼中
4. 無法分層管理 UI 優先權

### ✅ 新的解決方案

```cpp
// GameScene.cpp - 場景負責自己的 UI
bool GameScene::InitializeUI() {
    // 創建遊戲 HUD (持續層級)
    IUILayer* hudLayer = uiSystem_->CreatePersistentLayer("GameHUD", 400);
    
    // 創建遊戲介面 (場景層級)
    IUILayer* gameUI = uiSystem_->CreateLayer("GameInterface", UILayerType::Interface);
    
    // 背景圖片
    gameUI->CreateImage("bg.bmp", 50, 50, 200, 150, true);
    
    // 暫停按鈕 (事件驅動)
    gameUI->CreateButton("暫停", 10, 10, 80, 30, 
        [this](const UIEvent& event) { OnPauseButtonClicked(); });
}
```

## 基本使用方式

### 1. 初始化 UI 系統

```cpp
// 在 EngineContext 中初始化
class EngineContext {
private:
    std::unique_ptr<IUISystem> uiSystem_;
    
public:
    STDMETHODIMP Initialize(HWND hwnd, UINT width, UINT height) {
        // ... 其他初始化 ...
        
        // 初始化 UI 系統
        uiSystem_ = CreateUISystem();
        uiSystem_->Initialize(device.Get(), assetManager_.get());
        
        // 創建持續性層級
        CreatePersistentUILayers();
        
        return S_OK;
    }
    
private:
    void CreatePersistentUILayers() {
        // HUD 層級 (跨場景持續)
        auto* hudLayer = uiSystem_->CreatePersistentLayer("HUD", 400);
        
        // 除錯層級 (跨場景持續)
        auto* debugLayer = uiSystem_->CreatePersistentLayer("Debug", 450);
        
        // 系統層級 (最上層)
        auto* systemLayer = uiSystem_->CreatePersistentLayer("System", 500);
    }
};
```

### 2. 在場景中使用

```cpp
class GameScene : public Scene {
protected:
    bool OnInitialize() override {
        // 創建場景特定的 UI 層級
        gameUILayer_ = uiSystem_->CreateLayer("GameUI", UILayerType::Interface);
        overlayLayer_ = uiSystem_->CreateLayer("GameOverlay", UILayerType::Overlay);
        
        CreateGameUI();
        return true;
    }
    
    void CreateGameUI() {
        // 遊戲介面
        auto pauseBtn = gameUILayer_->CreateButton("暫停", 10, 10, 80, 30,
            [this](const UIEvent& event) {
                // 推送暫停場景
                // sceneManager->PushScene("PauseScene");
            });
            
        auto inventoryBtn = gameUILayer_->CreateButton("背包", 100, 10, 80, 30,
            [this](const UIEvent& event) {
                // 推送背包場景
                // sceneManager->PushScene("InventoryScene");
            });
            
        // 血量顯示 (HUD 層級 - 持續性)
        auto* hudLayer = uiSystem_->GetPersistentLayer("HUD");
        if (hudLayer) {
            healthBar_ = hudLayer->CreateSlider(20, 20, 200, 20, 0.0f, 100.0f, 100.0f);
            hudLayer->CreateText("HP", 5, 25, 30, 20, 0xFFFF0000);
        }
    }

private:
    IUILayer* gameUILayer_;
    IUILayer* overlayLayer_;
    std::string healthBar_;
};
```

### 3. 主迴圈整合

```cpp
STDMETHODIMP EngineContext::Run() {
    MSG msg = {};
    bool running = true;
    
    while (running) {
        // 處理 Windows 訊息
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // UI 系統處理輸入 (在場景處理之前)
        if (uiSystem_->HandleInput(msg)) {
            continue; // UI 處理了輸入，跳過場景處理
        }
        
        // 場景管理器處理輸入
        sceneManager_->HandleInput(msg);
        
        float deltaTime = 0.016f;
        
        // 更新系統
        sceneManager_->Update(deltaTime);
        uiSystem_->Update(deltaTime);
        
        // 渲染
        d3dContext_->Clear(...);
        d3dContext_->BeginScene();
        
        sceneManager_->Render();    // 先渲染場景
        uiSystem_->Render();        // 後渲染 UI (覆蓋在上面)
        
        d3dContext_->EndScene();
        d3dContext_->Present();
    }
    
    return S_OK;
}
```

## 進階功能

### 1. 動態 UI 管理

```cpp
class InventoryScene : public Scene {
protected:
    bool OnInitialize() override {
        // 創建透明覆蓋層 (顯示在遊戲場景上方)
        SetTransparent(true);
        
        inventoryLayer_ = uiSystem_->CreateLayer("Inventory", UILayerType::Overlay);
        
        CreateInventoryUI();
        return true;
    }
    
    void CreateInventoryUI() {
        // 半透明背景
        inventoryLayer_->CreateImage("inventory_bg.png", 100, 100, 600, 400);
        
        // 物品格子 (動態創建)
        for (int i = 0; i < 40; ++i) {
            int x = 120 + (i % 8) * 70;
            int y = 140 + (i / 8) * 70;
            
            std::string slotId = inventoryLayer_->CreateButton("", x, y, 60, 60,
                [this, i](const UIEvent& event) {
                    OnInventorySlotClicked(i, event);
                });
                
            inventorySlots_.push_back(slotId);
        }
        
        // 關閉按鈕
        inventoryLayer_->CreateButton("X", 670, 110, 20, 20,
            [this](const UIEvent& event) {
                // 彈出當前場景，回到遊戲
                // sceneManager->PopScene();
            });
    }
    
    void OnInventorySlotClicked(int slotIndex, const UIEvent& event) {
        // 處理物品點擊邏輯
        if (event.type == UIEventType::Click) {
            std::cout << "Clicked inventory slot: " << slotIndex << std::endl;
            // 顯示物品資訊、使用物品等
        }
    }

private:
    IUILayer* inventoryLayer_;
    std::vector<std::string> inventorySlots_;
};
```

### 2. 全域事件處理

```cpp
class UIEventHandler {
public:
    static void Initialize(IUISystem* uiSystem) {
        // 註冊全域事件處理器
        uiSystem->RegisterGlobalEventHandler(UIEventType::Click,
            [](const UIEvent& event) {
                std::cout << "Global click: " << event.componentId << std::endl;
                // 記錄點擊統計、音效播放等
            });
            
        uiSystem->RegisterGlobalEventHandler(UIEventType::ValueChanged,
            [](const UIEvent& event) {
                // 處理所有值變更事件
                if (event.componentId.find("volume") != std::string::npos) {
                    // 更新音量設定
                    AudioManager::SetVolume(event.floatValue);
                }
            });
    }
};
```

### 3. UI 層級管理

```cpp
class PauseScene : public Scene {
protected:
    bool OnInitialize() override {
        SetTransparent(true); // 透明場景，顯示背後的遊戲
        
        // 創建暫停選單層級 (高優先權)
        pauseLayer_ = uiSystem_->CreateLayer("PauseMenu", UILayerType::Overlay, 350);
        
        CreatePauseMenu();
        return true;
    }
    
    void OnSceneEnter() override {
        // 暫停時停用遊戲 UI 的互動
        auto* gameLayer = uiSystem_->GetLayer("GameUI");
        if (gameLayer) {
            gameLayer->SetInteractive(false);
        }
    }
    
    void OnSceneExit() override {
        // 恢復遊戲 UI 的互動
        auto* gameLayer = uiSystem_->GetLayer("GameUI");
        if (gameLayer) {
            gameLayer->SetInteractive(true);
        }
    }
    
    void CreatePauseMenu() {
        // 半透明覆蓋
        pauseLayer_->CreateImage("pause_overlay.png", 0, 0, 800, 600);
        
        // 暫停選單按鈕
        pauseLayer_->CreateButton("繼續遊戲", 350, 250, 100, 40,
            [this](const UIEvent& event) {
                // sceneManager->PopScene();
            });
            
        pauseLayer_->CreateButton("設定", 350, 300, 100, 40,
            [this](const UIEvent& event) {
                // sceneManager->PushScene("SettingsScene");
            });
            
        pauseLayer_->CreateButton("離開遊戲", 350, 350, 100, 40,
            [this](const UIEvent& event) {
                // sceneManager->SwitchToScene("MenuScene");
            });
    }

private:
    IUILayer* pauseLayer_;
};
```

## 除錯和監控

### 1. UI 系統狀態

```cpp
// 顯示 UI 系統狀態
uiSystem_->PrintLayerInfo();

// 輸出範例:
// === UISystem Debug Info ===
// Initialized: Yes
// Debug Mode: Enabled
// Total Layers: 5
// Total Components: 23
// Render Calls: 1247
// Focused: GameUI:pause_button
//
// Layer Stack (render order):
//   [0] Background (priority: 0) - 1 components
//   [1] GameUI (priority: 200) - 8 components
//   [2] HUD (priority: 400) [PERSISTENT] - 5 components
//   [3] PauseMenu (priority: 350) - 4 components
//   [4] Debug (priority: 450) [PERSISTENT] - 3 components
// ===========================
```

### 2. 層級資訊

```cpp
// 顯示特定層級的詳細資訊
auto* gameLayer = uiSystem_->GetLayer("GameUI");
if (gameLayer) {
    gameLayer->PrintComponentInfo();
}

// 輸出範例:
// === UILayer[GameUI] Components ===
// Layer Type: 2, Priority: 200
// Visible: Yes, Interactive: Yes
// Component Count: 8
//   GameUI_comp_1 [button] (10,10 80x30) text:'暫停' [HOVERED]
//   GameUI_comp_2 [button] (100,10 80x30) text:'背包'
//   GameUI_comp_3 [image] (50,50 200x150) image:'bg.bmp'
//   GameUI_comp_4 [text] (200,20 100x20) text:'Score: 1500'
// ======================================
```

## 與 ServiceLocator 整合

```cpp
// 更新 ServiceLocator 以包含 UISystem
class ServiceLocator : public IServiceLocator {
public:
    void SetUISystem(IUISystem* uiSystem) { uiSystem_ = uiSystem; }
    IUISystem* GetUISystem() const { return uiSystem_; }
    
    // ... 其他服務 ...

private:
    IUISystem* uiSystem_;
};

// 在場景中使用
class GameScene : public Scene {
protected:
    bool OnInitialize() override {
        // 從服務定位器獲取 UI 系統
        uiSystem_ = services_->GetUISystem();
        
        CreateGameUI();
        return true;
    }

private:
    IUISystem* uiSystem_;
};
```

## 優勢總結

1. **🎯 職責分離**: UI 創建從 EngineContext 移到場景中
2. **📱 分層管理**: 清晰的 UI 層級和優先權系統
3. **🔄 跨場景支援**: 持續性層級解決 HUD 等需求
4. **⚡ 事件驅動**: 類型安全的事件處理機制
5. **🛠️ 開發友善**: 豐富的除錯和監控功能
6. **🧩 模組化**: 每個場景管理自己的 UI，互不干擾

這個 UI 系統完全解決了您提到的核心問題，為現代遊戲 UI 開發提供了強大且靈活的基礎架構！