# SceneManager 系統使用指南

## 概述

新的 SceneManager 系統提供了完整的場景管理解決方案，解決了原來 EngineContext 承擔過多責任的問題。

## 核心概念

### 1. 場景 (Scene)
- **生命週期管理**: Initialize → Update → Render → Cleanup
- **狀態轉換**: OnEnter → OnExit → OnPause → OnResume
- **輸入處理**: 每個場景可以處理自己的輸入
- **資源管理**: 場景負責載入和清理自己的資源

### 2. 場景堆疊 (Scene Stack)
- **多層場景**: 支援場景堆疊，如：遊戲 + 暫停選單
- **透明度**: 場景可以設為透明，允許背後場景繼續渲染
- **輸入傳遞**: 輸入從最上層開始處理，透明場景允許向下傳遞

### 3. 場景轉換 (Scene Transition)
- **多種效果**: None、Fade、Slide、CrossFade、Custom
- **自訂轉換**: 支援自訂轉換效果和回調
- **異步處理**: 轉換過程中兩個場景可以並存

## 基本使用方式

### 初始化 SceneManager

```cpp
// 1. 建立服務定位器
auto serviceLocator = std::make_unique<ServiceLocator>();
serviceLocator->SetAssetManager(assetManager_.get());
serviceLocator->SetUIManager(uiManager_.get());
serviceLocator->SetConfigManager(configManager_.get());
serviceLocator->SetDevice(device.Get());

// 2. 建立場景管理器
sceneManager_ = CreateSceneManager();
sceneManager_->Initialize(serviceLocator.get());

// 3. 註冊場景
sceneManager_->RegisterScene("GameScene", []() {
    return std::make_unique<GameScene>();
});

sceneManager_->RegisterScene("MenuScene", []() {
    return std::make_unique<MenuScene>();
});

sceneManager_->RegisterScene("PauseScene", []() {
    return std::make_unique<PauseScene>();
});
```

### 場景切換

```cpp
// 立即切換到遊戲場景
sceneManager_->SwitchToScene("GameScene");

// 使用淡化效果切換
SceneTransitionParams transition;
transition.type = SceneTransitionType::Fade;
transition.duration = 1.0f;
sceneManager_->SwitchToScene("MenuScene", transition);

// 推送暫停場景到堆疊 (遊戲場景保持在背景)
sceneManager_->PushScene("PauseScene");

// 彈出當前場景，回到下一層
sceneManager_->PopScene();
```

### 主迴圈整合

```cpp
STDMETHODIMP EngineContext::Run() {
    MSG msg = {};
    bool running = true;
    
    while (running) {
        // 處理 Windows 訊息
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // 讓場景管理器處理輸入
        sceneManager_->HandleInput(msg);
        
        float deltaTime = 0.016f; // 60 FPS
        
        // 更新場景
        sceneManager_->Update(deltaTime);
        
        // 清除畫面
        d3dContext_->Clear(D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                          D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
        d3dContext_->BeginScene();
        
        // 渲染場景
        sceneManager_->Render();
        
        d3dContext_->EndScene();
        d3dContext_->Present();
    }
    
    return S_OK;
}
```

## 建立自訂場景

### 基本場景實作

```cpp
class MyCustomScene : public Scene {
public:
    MyCustomScene() : Scene("MyCustomScene") {
        // 設置場景屬性
        SetTransparent(false); // 不透明場景
    }

protected:
    bool OnInitialize() override {
        // 載入場景資產
        backgroundTexture_ = assetManager_->LoadTexture("background.bmp");
        
        // 建立場景 UI
        CreateSceneUI();
        
        return true;
    }
    
    void OnUpdate(float deltaTime) override {
        // 更新場景邏輯
        elapsedTime_ += deltaTime;
    }
    
    void OnRender() override {
        // 渲染場景內容
        RenderBackground();
        RenderGameObjects();
        // UI 自動渲染
    }
    
    void OnCleanup() override {
        // 清理場景資源
        backgroundTexture_.reset();
    }
    
    bool OnHandleInput(const MSG& msg) override {
        // 處理場景特定輸入
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_SPACE) {
            // 空白鍵處理
            return true;
        }
        return false;
    }

private:
    void CreateSceneUI() {
        // 建立場景特定的 UI
        auto* button = uiManager_->CreateButton(
            L"返回選單", 50, 50, 100, 40,
            [this]() { 
                // 切換回選單場景
                // 注意：需要通過某種方式取得 SceneManager 參考
            }
        );
    }
    
private:
    std::shared_ptr<IDirect3DTexture9> backgroundTexture_;
    float elapsedTime_ = 0.0f;
};
```

### 透明場景範例 (覆蓋式 UI)

```cpp
class PauseScene : public Scene {
public:
    PauseScene() : Scene("PauseScene") {
        SetTransparent(true); // 透明場景，顯示背後的遊戲場景
    }

protected:
    bool OnInitialize() override {
        // 建立暫停選單 UI
        CreatePauseMenu();
        return true;
    }
    
    void OnRender() override {
        // 渲染半透明背景
        RenderOverlay();
        // UI 自動渲染
    }
    
    bool OnHandleInput(const MSG& msg) override {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            // ESC 鍵關閉暫停選單
            // sceneManager_->PopScene();
            return true;
        }
        return false;
    }

private:
    void CreatePauseMenu() {
        // 建立暫停選單
        auto* resumeBtn = uiManager_->CreateButton(
            L"繼續遊戲", 350, 250, 100, 40,
            [this]() { /* 彈出暫停場景 */ }
        );
        
        auto* settingsBtn = uiManager_->CreateButton(
            L"設定", 350, 300, 100, 40,
            [this]() { /* 推送設定場景 */ }
        );
        
        auto* exitBtn = uiManager_->CreateButton(
            L"離開遊戲", 350, 350, 100, 40,
            [this]() { /* 切換到選單場景 */ }
        );
    }
};
```

## 場景間通訊

### 1. 通過服務傳遞資料

```cpp
// 在 ConfigManager 中存儲場景間的共享資料
configManager_->SetString("game.currentLevel", "Level1");
configManager_->SetInt("game.playerScore", 1500);

// 其他場景可以讀取
std::string currentLevel = configManager_->GetString("game.currentLevel");
int playerScore = configManager_->GetInt("game.playerScore");
```

### 2. 通過事件系統 (未來實作)

```cpp
// 發送場景事件
eventManager_->Publish(GameStateChangedEvent{
    .fromState = "Playing",
    .toState = "Paused"
});

// 監聽場景事件
eventManager_->Subscribe<GameStateChangedEvent>(
    [this](const GameStateChangedEvent& event) {
        // 處理遊戲狀態變化
    }
);
```

## 除錯和監控

### 場景堆疊資訊

```cpp
// 顯示當前場景堆疊
sceneManager_->PrintSceneStack();

// 輸出範例:
// === Scene Stack ===
// Stack Size: 2
//   [0] GameScene
//   [1] PauseScene (TRANSPARENT)
// ==================
```

### 場景狀態查詢

```cpp
// 檢查場景是否活躍
bool isGameActive = sceneManager_->IsSceneActive("GameScene");

// 獲取當前場景
IScene* currentScene = sceneManager_->GetCurrentScene();
if (currentScene) {
    std::cout << "Current scene: " << currentScene->GetName() << std::endl;
}

// 獲取所有已載入的場景
auto loadedScenes = sceneManager_->GetLoadedScenes();
for (const auto& sceneName : loadedScenes) {
    std::cout << "Loaded: " << sceneName << std::endl;
}
```

## 與原有系統的整合

### EngineContext 重構範例

```cpp
class EngineContext : public IEngineContext {
private:
    // 新的系統
    std::unique_ptr<ISceneManager> sceneManager_;
    std::unique_ptr<ServiceLocator> serviceLocator_;
    
    // 原有系統 (保持不變)
    std::unique_ptr<ID3DContext> d3dContext_;
    std::unique_ptr<IAssetManager> assetManager_;
    std::unique_ptr<IUIManager> uiManager_;
    std::unique_ptr<IConfigManager> configManager_;

public:
    STDMETHODIMP Initialize(HWND hwnd, UINT width, UINT height) override {
        // ... 原有的初始化 ...
        
        // 設置服務定位器
        serviceLocator_ = std::make_unique<ServiceLocator>();
        serviceLocator_->SetAssetManager(assetManager_.get());
        serviceLocator_->SetUIManager(uiManager_.get());
        serviceLocator_->SetConfigManager(configManager_.get());
        serviceLocator_->SetDevice(device.Get());
        
        // 初始化場景管理器
        sceneManager_ = CreateSceneManager();
        sceneManager_->Initialize(serviceLocator_.get());
        
        // 註冊場景
        RegisterScenes();
        
        // 載入初始場景
        std::string defaultScene = configManager_->GetString("scenes.defaultScene", "GameScene");
        sceneManager_->LoadScene(defaultScene);
        
        return S_OK;
    }
    
private:
    void RegisterScenes() {
        sceneManager_->RegisterScene("GameScene", []() {
            return std::make_unique<GameScene>();
        });
        
        sceneManager_->RegisterScene("MenuScene", []() {
            return std::make_unique<MenuScene>();
        });
        
        sceneManager_->RegisterScene("PauseScene", []() {
            return std::make_unique<PauseScene>();
        });
    }
};
```

## 優勢總結

1. **🎯 職責分離**: EngineContext 不再處理場景邏輯和 UI 創建
2. **🔄 場景重用**: 場景可以重複載入和卸載，記憶體效率高
3. **📚 堆疊管理**: 支援多層場景，如遊戲+暫停+設定
4. **🎬 轉換效果**: 豐富的場景轉換效果
5. **🛠️ 開發友善**: 清晰的場景生命週期和事件處理
6. **🧩 模組化**: 每個場景獨立管理自己的資源和邏輯

這個系統為 UI 系統重構和事件系統實作提供了良好的基礎架構。