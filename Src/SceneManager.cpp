#include "SceneManager.h"
#include <iostream>
#include <algorithm>

// Factory 函式實作
std::unique_ptr<ISceneManager> CreateSceneManager() {
    return std::make_unique<SceneManager>();
}

SceneManager::SceneManager()
    : services_(nullptr)
    , initialized_(false)
    , enableDebugLogging_(true)
{
}

SceneManager::~SceneManager() {
    Cleanup();
}

bool SceneManager::Initialize(IServiceLocator* services) {
    if (initialized_) {
        std::cerr << "SceneManager is already initialized" << std::endl;
        return false;
    }
    
    if (!services) {
        std::cerr << "SceneManager: Invalid service locator" << std::endl;
        return false;
    }
    
    services_ = services;
    initialized_ = true;
    
    std::cout << "SceneManager initialized successfully" << std::endl;
    return true;
}

void SceneManager::Cleanup() {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 清理所有場景
    PopAllScenes();
    
    // 清理轉換中的場景
    if (transitionToScene_) {
        CleanupScene(std::move(transitionToScene_));
    }
    
    // 清理註冊表
    sceneFactories_.clear();
    
    services_ = nullptr;
    initialized_ = false;
    
    std::cout << "SceneManager cleaned up" << std::endl;
}

void SceneManager::RegisterScene(const std::string& sceneName, SceneFactory factory) {
    if (!factory) {
        std::cerr << "SceneManager: Invalid factory for scene " << sceneName << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    sceneFactories_[sceneName] = factory;
    
    if (enableDebugLogging_) {
        std::cout << "Registered scene: " << sceneName << std::endl;
    }
}

bool SceneManager::UnregisterScene(const std::string& sceneName) {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    auto it = sceneFactories_.find(sceneName);
    if (it != sceneFactories_.end()) {
        sceneFactories_.erase(it);
        if (enableDebugLogging_) {
            std::cout << "Unregistered scene: " << sceneName << std::endl;
        }
        return true;
    }
    
    return false;
}

std::unique_ptr<IScene> SceneManager::CreateScene(const std::string& sceneName) {
    auto it = sceneFactories_.find(sceneName);
    if (it == sceneFactories_.end()) {
        std::cerr << "SceneManager: Scene factory not found: " << sceneName << std::endl;
        return nullptr;
    }
    
    auto scene = it->second();
    if (!scene) {
        std::cerr << "SceneManager: Failed to create scene: " << sceneName << std::endl;
        return nullptr;
    }
    
    // 初始化場景
    if (!scene->Initialize(services_)) {
        std::cerr << "SceneManager: Failed to initialize scene: " << sceneName << std::endl;
        return nullptr;
    }
    
    return scene;
}

bool SceneManager::LoadScene(const std::string& sceneName) {
    if (!initialized_) {
        std::cerr << "SceneManager not initialized" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 清理當前所有場景
    PopAllScenes();
    
    // 創建新場景
    auto scene = CreateScene(sceneName);
    if (!scene) {
        return false;
    }
    
    // 加入場景堆疊
    sceneStack_.emplace_back(std::move(scene));
    sceneStack_.back().scene->OnEnter();
    
    if (enableDebugLogging_) {
        std::cout << "Loaded scene: " << sceneName << std::endl;
    }
    
    return true;
}

bool SceneManager::SwitchToScene(const std::string& sceneName, 
                                const SceneTransitionParams& transition) {
    if (!initialized_) {
        std::cerr << "SceneManager not initialized" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 如果有轉換正在進行，取消它
    if (currentTransition_.active) {
        CompleteTransition();
    }
    
    // 創建目標場景
    auto newScene = CreateScene(sceneName);
    if (!newScene) {
        return false;
    }
    
    std::string fromSceneName = "";
    if (!sceneStack_.empty()) {
        fromSceneName = sceneStack_.back().scene->GetName();
    }
    
    if (transition.type == SceneTransitionType::None || transition.duration <= 0.0f) {
        // 立即切換
        PopAllScenes();
        sceneStack_.emplace_back(std::move(newScene));
        sceneStack_.back().scene->OnEnter();
        
        if (enableDebugLogging_) {
            std::cout << "Switched to scene: " << sceneName << std::endl;
        }
    } else {
        // 啟動轉換
        transitionToScene_ = std::move(newScene);
        StartTransition(fromSceneName, sceneName, transition);
    }
    
    return true;
}

bool SceneManager::PushScene(const std::string& sceneName,
                            const SceneTransitionParams& transition) {
    if (!initialized_) {
        std::cerr << "SceneManager not initialized" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 暫停當前場景
    PauseTopScene();
    
    // 創建新場景
    auto scene = CreateScene(sceneName);
    if (!scene) {
        // 恢復當前場景
        ResumeTopScene();
        return false;
    }
    
    // TODO: 支援轉換效果
    sceneStack_.emplace_back(std::move(scene));
    sceneStack_.back().scene->OnEnter();
    
    if (enableDebugLogging_) {
        std::cout << "Pushed scene: " << sceneName << " (stack size: " << sceneStack_.size() << ")" << std::endl;
    }
    
    return true;
}

bool SceneManager::PopScene(const SceneTransitionParams& transition) {
    if (!initialized_) {
        std::cerr << "SceneManager not initialized" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    if (sceneStack_.empty()) {
        std::cerr << "SceneManager: No scenes to pop" << std::endl;
        return false;
    }
    
    // 移除最上層場景
    auto& topItem = sceneStack_.back();
    std::string sceneName = topItem.scene->GetName();
    
    topItem.scene->OnExit();
    CleanupScene(std::move(topItem.scene));
    sceneStack_.pop_back();
    
    // 恢復下一層場景
    ResumeTopScene();
    
    if (enableDebugLogging_) {
        std::cout << "Popped scene: " << sceneName << " (stack size: " << sceneStack_.size() << ")" << std::endl;
    }
    
    return true;
}

void SceneManager::PopAllScenes() {
    while (!sceneStack_.empty()) {
        auto& topItem = sceneStack_.back();
        topItem.scene->OnExit();
        CleanupScene(std::move(topItem.scene));
        sceneStack_.pop_back();
    }
    
    if (enableDebugLogging_ && !sceneStack_.empty()) {
        std::cout << "Popped all scenes" << std::endl;
    }
}

IScene* SceneManager::GetCurrentScene() const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    return sceneStack_.empty() ? nullptr : sceneStack_.back().scene.get();
}

IScene* SceneManager::GetScene(const std::string& sceneName) const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    for (const auto& item : sceneStack_) {
        if (item.scene->GetName() == sceneName) {
            return item.scene.get();
        }
    }
    
    return nullptr;
}

bool SceneManager::HasScene(const std::string& sceneName) const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    return sceneFactories_.find(sceneName) != sceneFactories_.end();
}

std::vector<std::string> SceneManager::GetLoadedScenes() const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    std::vector<std::string> result;
    for (const auto& item : sceneStack_) {
        result.push_back(item.scene->GetName());
    }
    
    return result;
}

bool SceneManager::IsSceneActive(const std::string& sceneName) const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    for (const auto& item : sceneStack_) {
        if (item.scene->GetName() == sceneName) {
            return !item.isPaused;
        }
    }
    
    return false;
}

size_t SceneManager::GetSceneStackSize() const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    return sceneStack_.size();
}

void SceneManager::Update(float deltaTime) {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 更新轉換
    if (currentTransition_.active) {
        UpdateTransition(deltaTime);
    }
    
    // 更新場景堆疊 (從底層到頂層)
    for (auto& item : sceneStack_) {
        if (!item.isPaused) {
            item.scene->Update(deltaTime);
        }
    }
}

void SceneManager::Render() {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    if (currentTransition_.active) {
        RenderTransition();
    } else {
        RenderSceneStack();
    }
}

bool SceneManager::HandleInput(const MSG& msg) {
    if (!initialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    // 從最上層場景開始處理輸入
    for (auto it = sceneStack_.rbegin(); it != sceneStack_.rend(); ++it) {
        if (!it->isPaused && it->scene->HandleInput(msg)) {
            return true; // 輸入被處理了
        }
        
        // 如果場景不透明，停止向下傳遞
        if (!it->scene->IsTransparent()) {
            break;
        }
    }
    
    return false;
}

void SceneManager::PrintSceneStack() const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    
    std::cout << "\n=== Scene Stack ===" << std::endl;
    std::cout << "Stack Size: " << sceneStack_.size() << std::endl;
    
    for (size_t i = 0; i < sceneStack_.size(); ++i) {
        const auto& item = sceneStack_[i];
        std::cout << "  [" << i << "] " << item.scene->GetName();
        
        if (item.isPaused) {
            std::cout << " (PAUSED)";
        }
        
        if (item.scene->IsTransparent()) {
            std::cout << " (TRANSPARENT)";
        }
        
        std::cout << std::endl;
    }
    
    if (currentTransition_.active) {
        std::cout << "Transition: " << currentTransition_.fromScene 
                  << " -> " << currentTransition_.toScene 
                  << " (" << (currentTransition_.getProgress() * 100.0f) << "%)" << std::endl;
    }
    
    std::cout << "==================\n" << std::endl;
}

// 私有輔助方法實作
void SceneManager::UpdateTransition(float deltaTime) {
    if (!currentTransition_.active) {
        return;
    }
    
    currentTransition_.elapsed += deltaTime;
    
    if (currentTransition_.customCallback) {
        currentTransition_.customCallback(currentTransition_.getProgress());
    }
    
    if (currentTransition_.isComplete()) {
        CompleteTransition();
    }
}

void SceneManager::StartTransition(const std::string& fromScene, const std::string& toScene,
                                  const SceneTransitionParams& params) {
    currentTransition_.active = true;
    currentTransition_.type = params.type;
    currentTransition_.duration = params.duration;
    currentTransition_.elapsed = 0.0f;
    currentTransition_.fromScene = fromScene;
    currentTransition_.toScene = toScene;
    currentTransition_.customCallback = params.customTransition;
    
    if (enableDebugLogging_) {
        std::cout << "Started transition: " << fromScene << " -> " << toScene << std::endl;
    }
}

void SceneManager::CompleteTransition() {
    if (!currentTransition_.active) {
        return;
    }
    
    // 完成轉換
    if (transitionToScene_) {
        PopAllScenes();
        sceneStack_.emplace_back(std::move(transitionToScene_));
        sceneStack_.back().scene->OnEnter();
        
        if (enableDebugLogging_) {
            std::cout << "Completed transition to: " << currentTransition_.toScene << std::endl;
        }
    }
    
    // 清理轉換狀態
    currentTransition_ = SceneTransition{};
    transitionToScene_.reset();
}

void SceneManager::PauseTopScene() {
    if (!sceneStack_.empty()) {
        auto& topItem = sceneStack_.back();
        if (!topItem.isPaused) {
            topItem.isPaused = true;
            topItem.scene->OnPause();
        }
    }
}

void SceneManager::ResumeTopScene() {
    if (!sceneStack_.empty()) {
        auto& topItem = sceneStack_.back();
        if (topItem.isPaused) {
            topItem.isPaused = false;
            topItem.scene->OnResume();
        }
    }
}

void SceneManager::CleanupScene(std::unique_ptr<IScene> scene) {
    if (scene) {
        scene->Cleanup();
        scene.reset();
    }
}

void SceneManager::RenderTransition() {
    // 簡單的轉換實作 - 可以根據需要擴展
    float progress = currentTransition_.getProgress();
    
    switch (currentTransition_.type) {
        case SceneTransitionType::Fade:
            // 渲染舊場景
            if (!sceneStack_.empty()) {
                sceneStack_.back().scene->Render();
            }
            // TODO: 應用淡化效果
            break;
            
        case SceneTransitionType::CrossFade:
            // 渲染兩個場景並混合
            if (!sceneStack_.empty()) {
                sceneStack_.back().scene->Render();
            }
            if (transitionToScene_) {
                // TODO: 應用透明度混合
                transitionToScene_->Render();
            }
            break;
            
        default:
            // 預設只渲染當前場景
            RenderSceneStack();
            break;
    }
}

void SceneManager::RenderSceneStack() {
    // 從底層開始渲染，但只渲染到第一個不透明的場景
    for (const auto& item : sceneStack_) {
        item.scene->Render();
        
        // 如果場景不透明，不需要渲染更深層的場景
        if (!item.scene->IsTransparent()) {
            break;
        }
    }
}