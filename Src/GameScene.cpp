#include "GameScene.h"
#include "IAssetManager.h"
#include "IUIManager.h"
#include "IConfigManager.h"
#include "LightManager.h"
#include "CameraController.h"
#include "Scene3D.h"
#include "DirectionalLight.h"
#include "ModelData.h"
#include <iostream>

GameScene::GameScene()
    : Scene("GameScene")
    , elapsedTime_(0.0f)
    , showDebugInfo_(false)
    , pauseButtonId_(-1)
    , settingsButtonId_(-1)
    , helpButtonId_(-1)
    , hudLayerId_(-1)
    , screenWidth_(800)
    , screenHeight_(600)
{
    // 遊戲場景通常不透明
    SetTransparent(false);
}

bool GameScene::OnInitialize() {
    std::cout << "Initializing GameScene..." << std::endl;
    
    // 從配置讀取螢幕尺寸
    if (configManager_) {
        screenWidth_ = configManager_->GetInt("graphics.width", 800);
        screenHeight_ = configManager_->GetInt("graphics.height", 600);
        showDebugInfo_ = configManager_->GetBool("debug.showFPS", true);
    }
    
    // 按順序初始化各個子系統
    if (!InitializeAssets()) {
        std::cerr << "GameScene: Failed to initialize assets" << std::endl;
        return false;
    }
    
    if (!InitializeLighting()) {
        std::cerr << "GameScene: Failed to initialize lighting" << std::endl;
        return false;
    }
    
    if (!InitializeCamera()) {
        std::cerr << "GameScene: Failed to initialize camera" << std::endl;
        return false;
    }
    
    if (!InitializeUI()) {
        std::cerr << "GameScene: Failed to initialize UI" << std::endl;
        return false;
    }
    
    std::cout << "GameScene initialized successfully" << std::endl;
    return true;
}

bool GameScene::InitializeAssets() {
    // 載入遊戲場景需要的資產
    horseModel_ = assetManager_->LoadModel("horse_group.x");
    if (!horseModel_) {
        std::cerr << "GameScene: Failed to load horse model" << std::endl;
        return false;
    }
    
    horseTexture_ = assetManager_->LoadTexture("Horse2.bmp");
    if (!horseTexture_) {
        std::cerr << "GameScene: Failed to load horse texture" << std::endl;
        return false;
    }
    
    std::cout << "GameScene: Assets loaded successfully" << std::endl;
    return true;
}

bool GameScene::InitializeLighting() {
    // 創建光照管理器
    lightManager_ = CreateLightManager();
    if (!lightManager_) {
        return false;
    }
    
    // 添加基本的方向光
    auto dirLight = std::make_unique<DirectionalLight>(
        1.0f, 1.0f, 1.0f,     // 白色光
        -0.577f, -0.577f, 0.577f  // 光線方向
    );
    lightManager_->AddLight(dirLight.release());
    
    std::cout << "GameScene: Lighting initialized" << std::endl;
    return true;
}

bool GameScene::InitializeCamera() {
    // 創建相機控制器
    cameraController_ = CreateCameraController(device_, screenWidth_, screenHeight_);
    if (!cameraController_) {
        return false;
    }
    
    std::cout << "GameScene: Camera initialized" << std::endl;
    return true;
}

bool GameScene::InitializeUI() {
    // 創建 HUD 層級
    hudLayerId_ = uiManager_->CreateLayer(L"GameHUD", 1.0f);
    
    // 創建遊戲 UI - 使用配置化的方式而非硬編碼
    
    // 背景圖片 (可拖曳)
    auto* bgImage = uiManager_->CreateImage(L"bg.bmp", 50, 50, 200, 150, true);
    
    // 暫停按鈕
    auto* pauseButton = uiManager_->CreateButton(
        L"暫停", 10, 10, 80, 30,
        [this]() { OnPauseButtonClicked(); },
        bgImage,
        L"bt.bmp"
    );
    
    // 設定按鈕  
    auto* settingsButton = uiManager_->CreateButton(
        L"設定", 10, 50, 80, 30,
        [this]() { OnSettingsButtonClicked(); },
        bgImage,
        L"bt.bmp"
    );
    
    // 說明按鈕
    auto* helpButton = uiManager_->CreateButton(
        L"說明", 10, 90, 80, 30,
        [this]() { OnHelpButtonClicked(); },
        bgImage,
        L"bt.bmp"
    );
    
    // 遊戲資訊文字
    uiManager_->AddText(L"遊戲場景", 10, 10, 200, 30, 0xFFFFFFFF, hudLayerId_);
    
    if (showDebugInfo_) {
        uiManager_->AddText(L"FPS: 60", 10, screenHeight_ - 50, 100, 30, 0xFF00FF00, hudLayerId_);
        uiManager_->AddText(L"場景: GameScene", 10, screenHeight_ - 80, 200, 30, 0xFF00FF00, hudLayerId_);
    }
    
    std::cout << "GameScene: UI initialized" << std::endl;
    return true;
}

void GameScene::OnUpdate(float deltaTime) {
    elapsedTime_ += deltaTime;
    
    // 更新相機
    if (cameraController_) {
        cameraController_->Update(deltaTime);
    }
    
    // 更新場景特定的邏輯
    // TODO: 更新遊戲物件、動畫等
}

void GameScene::OnRender() {
    // 應用光照
    if (lightManager_) {
        lightManager_->ApplyAll(device_);
    }
    
    // 渲染 3D 場景
    if (cameraController_ && horseModel_) {
        // 計算視圖和投影矩陣
        float aspect = static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_);
        auto viewMatrix = cameraController_->GetViewMatrix();
        auto projMatrix = cameraController_->GetProjMatrix(aspect);
        
        // TODO: 渲染 3D 模型
        // 這裡需要更詳細的渲染邏輯
    }
    
    // UI 由 UIManager 自動渲染，不需要在這裡處理
}

void GameScene::OnCleanup() {
    std::cout << "Cleaning up GameScene..." << std::endl;
    
    // 清理子系統
    lightManager_.reset();
    cameraController_.reset();
    scene3D_.reset();
    
    // 清理資產參考
    horseModel_.reset();
    horseTexture_.reset();
    
    // 清理 UI（UIManager 會自動處理）
    
    std::cout << "GameScene cleaned up" << std::endl;
}

void GameScene::OnSceneEnter() {
    std::cout << "Entering GameScene" << std::endl;
    
    // 場景進入時的特殊處理
    if (configManager_ && configManager_->GetBool("debug.enableLogging", true)) {
        std::cout << "GameScene: Debug logging enabled" << std::endl;
    }
}

void GameScene::OnSceneExit() {
    std::cout << "Exiting GameScene" << std::endl;
    
    // 場景退出時的特殊處理
    // 例如：保存遊戲狀態、釋放資源等
}

bool GameScene::OnHandleInput(const MSG& msg) {
    // 處理遊戲特定的輸入
    switch (msg.message) {
        case WM_KEYDOWN:
            switch (msg.wParam) {
                case VK_ESCAPE:
                    OnPauseButtonClicked();
                    return true;
                    
                case VK_F1:
                    OnHelpButtonClicked();
                    return true;
                    
                case 'D':
                case 'd':
                    showDebugInfo_ = !showDebugInfo_;
                    std::cout << "Debug info " << (showDebugInfo_ ? "enabled" : "disabled") << std::endl;
                    return true;
            }
            break;
    }
    
    // 讓相機處理輸入
    if (cameraController_) {
        // 這裡需要適配 ICameraController 的輸入處理介面
        // cameraController_->HandleInput(msg);
    }
    
    return false;
}

// UI 事件處理方法
void GameScene::OnPauseButtonClicked() {
    std::cout << "GameScene: Pause button clicked" << std::endl;
    
    // TODO: 推送暫停場景到堆疊
    // sceneManager->PushScene("PauseScene");
    
    // 暫時顯示訊息框
    MessageBoxA(nullptr, "遊戲暫停!\n(這裡應該推送暫停場景)", "GameScene", MB_OK);
}

void GameScene::OnSettingsButtonClicked() {
    std::cout << "GameScene: Settings button clicked" << std::endl;
    
    // TODO: 推送設定場景到堆疊
    // sceneManager->PushScene("SettingsScene");
    
    MessageBoxA(nullptr, "打開設定!\n(這裡應該推送設定場景)", "GameScene", MB_OK);
}

void GameScene::OnHelpButtonClicked() {
    std::cout << "GameScene: Help button clicked" << std::endl;
    
    // TODO: 推送說明場景到堆疊
    // sceneManager->PushScene("HelpScene");
    
    MessageBoxA(nullptr, "顯示說明!\n(這裡應該推送說明場景)", "GameScene", MB_OK);
}