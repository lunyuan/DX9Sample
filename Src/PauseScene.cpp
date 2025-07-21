#include "PauseScene.h"
#include "SettingsScene.h"
// #include "IUISystem.h" // Removed - using UIManager only
#include "IUIManager.h"
#include "IAssetManager.h"
#include "IConfigManager.h"
#include "ISceneManager.h"
#include <iostream>

PauseScene::PauseScene()
    : Scene("PauseScene")
    , EventListener(nullptr)
    , pauseMenuLayerId_(-1)
    , resumeButtonPtr_(nullptr)
    , settingsButtonPtr_(nullptr)
    , quitButtonPtr_(nullptr)
    , fadeInTime_(0.3f)
    , currentAlpha_(0.0f)
    , isVisible_(false)
{
    
    // 暫停場景是透明的，允許背後的場景繼續渲染
    SetTransparent(true);
}

bool PauseScene::OnInitialize() {
    if (!Scene::OnInitialize()) {
        return false;
    }
    
    // 設置 EventListener
    auto* eventManager = services_->GetEventManager();
    if (eventManager) {
        SetEventManager(eventManager);
        
        // 註冊事件處理器
        LISTEN_TO_EVENT(Events::UIComponentClicked, OnUIComponentClicked);
        
    }
    
    // 創建暫停菜單 UI
    CreatePauseMenu();
    
    return true;
}

void PauseScene::OnUpdate(float deltaTime) {
    Scene::OnUpdate(deltaTime);
    
    // 處理淡入動畫
    if (isVisible_ && currentAlpha_ < 1.0f) {
        currentAlpha_ += deltaTime / fadeInTime_;
        if (currentAlpha_ > 1.0f) {
            currentAlpha_ = 1.0f;
        }
        
        // UIManager 目前不支援動態調整透明度
    }
}

void PauseScene::OnRender() {
    Scene::OnRender();
    // 暫停場景的特定渲染邏輯可以在這裡添加
    // 例如：特殊效果、動畫等
}

void PauseScene::OnCleanup() {
    // 清理 UI 層級和元素
    if (services_ && services_->GetUIManager()) {
        auto* uiManager = services_->GetUIManager();
        if (pauseMenuLayerId_ >= 0) {
            uiManager->ClearLayer(pauseMenuLayerId_);
            pauseMenuLayerId_ = -1;
        }
    }
    
    // 清理指標
    resumeButtonPtr_ = nullptr;
    settingsButtonPtr_ = nullptr;
    quitButtonPtr_ = nullptr;
    
    Scene::OnCleanup();
}

void PauseScene::OnEnter() {
    Scene::OnEnter();
    
    // 啟動淡入動畫
    isVisible_ = true;
    currentAlpha_ = 0.0f;
    
    // 暫停背景音樂或遊戲聲音
    // 這裡可以發送音頻控制事件
    
    // 發送暫停事件
    if (services_->GetEventManager()) {
        Events::GameStateChanged pauseEvent;
        pauseEvent.previousState = "playing";
        pauseEvent.newState = "paused";
        pauseEvent.transitionTime = fadeInTime_;
        Emit(pauseEvent);
    }
    
}

void PauseScene::OnExit() {
    // 清理 UI 層級和元素，避免按鈕回調引用無效的場景
    if (services_ && services_->GetUIManager()) {
        auto* uiManager = services_->GetUIManager();
        if (pauseMenuLayerId_ >= 0) {
            uiManager->ClearLayer(pauseMenuLayerId_);
        }
    }
    
    Scene::OnExit();
    
    isVisible_ = false;
    
    // 恢復背景音樂或遊戲聲音
    // 發送恢復事件
    if (services_ && services_->GetEventManager()) {
        Events::GameStateChanged resumeEvent;
        resumeEvent.previousState = "paused";
        resumeEvent.newState = "playing";
        resumeEvent.transitionTime = 0.0f;
        Emit(resumeEvent);
    }
    
}

bool PauseScene::OnHandleInput(const MSG& msg) {
    // 處理特殊按鍵
    if (msg.message == WM_KEYDOWN) {
        switch (msg.wParam) {
            case VK_ESCAPE:
                // ESC 鍵恢復遊戲
                return true;
                
            case VK_RETURN:
                // Enter 鍵也恢復遊戲
                return true;
        }
    }
    
    // 讓基類處理其他輸入（包括 UI 輸入）
    return Scene::OnHandleInput(msg);
}

void PauseScene::CreatePauseMenu() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        std::cerr << "PauseScene: UIManager not available" << std::endl;
        return;
    }
    
    // 創建暫停菜單層級（高優先權）
    pauseMenuLayerId_ = uiManager->CreateLayer(L"PauseMenu", 3.0f, 1.0f);
    
    // 創建對話框背景圖片 (不可拖曳)
    // 對話框位置: x=400, y=150, width=480, height=420
    int dialogX = 400;
    int dialogY = 150;
    int dialogWidth = 480;
    int dialogHeight = 420;
    int dialogCenterX = dialogX + dialogWidth / 2;
    
    auto* bgImage = uiManager->CreateImage(L"dialog1.bmp", dialogX, dialogY, dialogWidth, dialogHeight, false);
    
    // 創建標題文字
    uiManager->AddText(L"GAME PAUSED", dialogCenterX - 60, dialogY + 30, 120, 40, 0xFFFFFFFF, pauseMenuLayerId_);
    
    // 按鈕設定
    int buttonWidth = 140;
    int buttonHeight = 40;
    int buttonX = dialogCenterX - (buttonWidth / 2);
    
    // 創建按鈕 - 使用 UIManager 的 CreateButton
    auto* resumeButton = uiManager->CreateButton(
        L"Resume", buttonX, dialogY + 130, buttonWidth, buttonHeight,
        [this]() { 
            // 恢復遊戲
            if (services_ && services_->GetSceneManager()) {
                services_->GetSceneManager()->PopScene();
            }
        }, nullptr, L"bt.bmp"
    );
    
    auto* settingsButton = uiManager->CreateButton(
        L"Settings", buttonX, dialogY + 190, buttonWidth, buttonHeight,
        [this]() {
            // 切換到設定場景
            if (this && this->services_ && this->services_->GetSceneManager()) {
                this->services_->GetSceneManager()->PushScene("SettingsScene");
            }
        }, nullptr, L"bt.bmp"
    );
    
    auto* quitButton = uiManager->CreateButton(
        L"Quit", buttonX, dialogY + 250, buttonWidth, buttonHeight,
        []() {
            // 退出遊戲
            PostQuitMessage(0);
        }, nullptr, L"bt.bmp"
    );
    
    // 說明文字
    uiManager->AddText(L"Press ESC to resume", dialogCenterX - 80, dialogY + 330, 160, 20, 0xFF888888, pauseMenuLayerId_);
}

void PauseScene::OnUIComponentClicked(const Events::UIComponentClicked& event) {
    // UIManager 使用回調函式處理點擊，所以這個函式可能不會被呼叫
    OutputDebugStringA(("PauseScene: Received click event for component: " + event.componentId + "\n").c_str());
}


// Factory 函式
std::unique_ptr<IScene> CreatePauseScene() {
    return std::make_unique<PauseScene>();
}