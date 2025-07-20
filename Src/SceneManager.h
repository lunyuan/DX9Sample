#pragma once

#include "ISceneManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

// 場景轉換狀態
struct SceneTransition {
    bool active = false;
    SceneTransitionType type = SceneTransitionType::None;
    float duration = 0.0f;
    float elapsed = 0.0f;
    std::string fromScene;
    std::string toScene;
    std::function<void(float)> customCallback;
    
    bool isComplete() const { return elapsed >= duration; }
    float getProgress() const { return duration > 0.0f ? elapsed / duration : 1.0f; }
};

// 場景堆疊項目
struct SceneStackItem {
    std::unique_ptr<IScene> scene;
    bool isPaused = false;
    
    SceneStackItem(std::unique_ptr<IScene> s) : scene(std::move(s)) {}
};

class SceneManager : public ISceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    // ISceneManager 介面實作
    void RegisterScene(const std::string& sceneName, SceneFactory factory) override;
    bool UnregisterScene(const std::string& sceneName) override;
    
    bool LoadScene(const std::string& sceneName) override;
    bool SwitchToScene(const std::string& sceneName, 
                      const SceneTransitionParams& transition = {}) override;
    
    bool PushScene(const std::string& sceneName,
                  const SceneTransitionParams& transition = {}) override;
    bool PopScene(const SceneTransitionParams& transition = {}) override;
    void PopAllScenes() override;
    
    IScene* GetCurrentScene() const override;
    IScene* GetScene(const std::string& sceneName) const override;
    bool HasScene(const std::string& sceneName) const override;
    std::vector<std::string> GetLoadedScenes() const override;
    
    bool IsSceneActive(const std::string& sceneName) const override;
    size_t GetSceneStackSize() const override;
    
    void Update(float deltaTime) override;
    void Render() override;
    bool HandleInput(const MSG& msg) override;
    
    bool Initialize(IServiceLocator* services) override;
    void Cleanup() override;
    
    void PrintSceneStack() const override;

private:
    // 內部輔助方法
    std::unique_ptr<IScene> CreateScene(const std::string& sceneName);
    void UpdateTransition(float deltaTime);
    void StartTransition(const std::string& fromScene, const std::string& toScene,
                        const SceneTransitionParams& params);
    void CompleteTransition();
    
    // 場景堆疊管理
    void PauseTopScene();
    void ResumeTopScene();
    void CleanupScene(std::unique_ptr<IScene> scene);
    
    // 渲染輔助
    void RenderTransition();
    void RenderSceneStack();

private:
    // 服務
    IServiceLocator* services_;
    
    // 場景註冊表
    std::unordered_map<std::string, SceneFactory> sceneFactories_;
    
    // 場景堆疊 (最後一個是最上層)
    std::vector<SceneStackItem> sceneStack_;
    
    // 場景轉換
    SceneTransition currentTransition_;
    std::unique_ptr<IScene> transitionToScene_;
    
    // 執行緒安全
    mutable std::mutex sceneMutex_;
    
    // 狀態
    bool initialized_;
    
    // 除錯
    bool enableDebugLogging_;
};