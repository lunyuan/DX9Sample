#pragma once

#include "IScene.h"
#include <memory>
#include <string>
#include <functional>

// 場景轉換類型
enum class SceneTransitionType {
    None,           // 立即切換
    Fade,           // 淡入淡出
    Slide,          // 滑動
    CrossFade,      // 交叉淡化
    Custom          // 自訂轉換
};

// 場景轉換參數
struct SceneTransitionParams {
    SceneTransitionType type = SceneTransitionType::None;
    float duration = 0.5f;          // 轉換時間（秒）
    bool keepPreviousActive = false; // 是否保持前一場景活躍
    
    // 自訂轉換回調
    std::function<void(float progress)> customTransition;
};

// 場景工廠函式類型
using SceneFactory = std::function<std::unique_ptr<IScene>()>;

// 場景管理器介面
struct ISceneManager {
    virtual ~ISceneManager() = default;
    
    // 場景註冊
    virtual void RegisterScene(const std::string& sceneName, SceneFactory factory) = 0;
    virtual bool UnregisterScene(const std::string& sceneName) = 0;
    
    // 場景載入和切換
    virtual bool LoadScene(const std::string& sceneName) = 0;
    virtual bool SwitchToScene(const std::string& sceneName, 
                              const SceneTransitionParams& transition = {}) = 0;
    
    // 場景堆疊管理 (支援多層場景，如：遊戲+暫停選單)
    virtual bool PushScene(const std::string& sceneName,
                          const SceneTransitionParams& transition = {}) = 0;
    virtual bool PopScene(const SceneTransitionParams& transition = {}) = 0;
    virtual void PopAllScenes() = 0;
    
    // 場景查詢
    virtual IScene* GetCurrentScene() const = 0;
    virtual IScene* GetScene(const std::string& sceneName) const = 0;
    virtual bool HasScene(const std::string& sceneName) const = 0;
    virtual std::vector<std::string> GetLoadedScenes() const = 0;
    
    // 場景狀態
    virtual bool IsSceneActive(const std::string& sceneName) const = 0;
    virtual size_t GetSceneStackSize() const = 0;
    
    // 系統更新
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual bool HandleInput(const MSG& msg) = 0;
    
    // 初始化和清理
    virtual bool Initialize(IServiceLocator* services) = 0;
    virtual void Cleanup() = 0;
    
    // 除錯
    virtual void PrintSceneStack() const = 0;
};

// Factory 函式
std::unique_ptr<ISceneManager> CreateSceneManager();