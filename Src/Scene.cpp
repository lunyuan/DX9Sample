#include "Scene.h"
#include "IAssetManager.h"
#include "IUIManager.h"
#include "IConfigManager.h"
#include <iostream>

Scene::Scene(const std::string& name)
    : name_(name)
    , state_(SceneState::Uninitialized)
    , isTransparent_(false)
    , initialized_(false)
    , services_(nullptr)
    , assetManager_(nullptr)
    , uiManager_(nullptr)
    , eventManager_(nullptr)
    , configManager_(nullptr)
    , device_(nullptr)
{
}

bool Scene::Initialize(IServiceLocator* services) {
    if (initialized_) {
        std::cerr << "Scene " << name_ << " is already initialized" << std::endl;
        return false;
    }
    
    if (!services) {
        std::cerr << "Scene " << name_ << ": Invalid service locator" << std::endl;
        return false;
    }
    
    SetState(SceneState::Initializing);
    
    // 保存服務參考
    services_ = services;
    assetManager_ = services->GetAssetManager();
    uiManager_ = services->GetUIManager();
    eventManager_ = services->GetEventManager();
    configManager_ = services->GetConfigManager();
    device_ = services->GetDevice();
    
    // 驗證必要服務
    if (!assetManager_ || !uiManager_ || !device_) {
        std::cerr << "Scene " << name_ << ": Missing required services" << std::endl;
        SetState(SceneState::Uninitialized);
        return false;
    }
    
    // 調用子類的初始化
    if (!OnInitialize()) {
        std::cerr << "Scene " << name_ << ": OnInitialize failed" << std::endl;
        SetState(SceneState::Uninitialized);
        return false;
    }
    
    initialized_ = true;
    SetState(SceneState::Running);
    
    std::cout << "Scene " << name_ << " initialized successfully" << std::endl;
    return true;
}

void Scene::Update(float deltaTime) {
    if (state_ != SceneState::Running) {
        return;
    }
    
    // 調用子類的更新
    OnUpdate(deltaTime);
}

void Scene::Render() {
    if (state_ != SceneState::Running && state_ != SceneState::Paused) {
        return;
    }
    
    // 調用子類的渲染
    OnRender();
}

void Scene::Cleanup() {
    if (!initialized_) {
        return;
    }
    
    SetState(SceneState::Cleanup);
    
    // 調用子類的清理
    OnCleanup();
    
    // 清理服務參考
    services_ = nullptr;
    assetManager_ = nullptr;
    uiManager_ = nullptr;
    eventManager_ = nullptr;
    configManager_ = nullptr;
    device_ = nullptr;
    
    initialized_ = false;
    SetState(SceneState::Uninitialized);
    
    std::cout << "Scene " << name_ << " cleaned up" << std::endl;
}

void Scene::OnEnter() {
    std::cout << "Scene " << name_ << " entered" << std::endl;
    OnSceneEnter();
}

void Scene::OnExit() {
    std::cout << "Scene " << name_ << " exited" << std::endl;
    OnSceneExit();
}

void Scene::OnPause() {
    if (state_ == SceneState::Running) {
        SetState(SceneState::Paused);
        std::cout << "Scene " << name_ << " paused" << std::endl;
        OnScenePause();
    }
}

void Scene::OnResume() {
    if (state_ == SceneState::Paused) {
        SetState(SceneState::Running);
        std::cout << "Scene " << name_ << " resumed" << std::endl;
        OnSceneResume();
    }
}

bool Scene::HandleInput(const MSG& msg) {
    if (state_ != SceneState::Running) {
        return false;
    }
    
    // 先讓 UI 處理輸入
    if (uiManager_ && uiManager_->HandleMessage(msg)) {
        return true;
    }
    
    // 然後讓場景處理輸入
    return OnHandleInput(msg);
}