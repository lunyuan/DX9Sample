#pragma once

#include "IScene.h"
#include <string>

// 場景基類 - 提供通用實作
class Scene : public IScene {
public:
    explicit Scene(const std::string& name);
    virtual ~Scene() = default;
    
    // IScene 介面實作
    bool Initialize(IServiceLocator* services) override;
    void Update(float deltaTime) override;
    void Render() override;
    void Cleanup() override;
    
    void OnEnter() override;
    void OnExit() override;
    void OnPause() override;
    void OnResume() override;
    
    const std::string& GetName() const override { return name_; }
    SceneState GetState() const override { return state_; }
    bool IsTransparent() const override { return isTransparent_; }
    
    bool HandleInput(const MSG& msg) override;

protected:
    void SetState(SceneState state) override { state_ = state; }
    
    // 子類需要實作的虛擬方法
    virtual bool OnInitialize() = 0;      // 場景特定的初始化
    virtual void OnUpdate(float deltaTime) = 0;  // 場景特定的更新
    virtual void OnRender() = 0;          // 場景特定的渲染
    virtual void OnCleanup() = 0;         // 場景特定的清理
    
    // 可選的覆寫方法
    virtual void OnSceneEnter() {}        // 場景進入時的額外處理
    virtual void OnSceneExit() {}         // 場景退出時的額外處理
    virtual void OnScenePause() {}        // 場景暫停時的額外處理
    virtual void OnSceneResume() {}       // 場景恢復時的額外處理
    
    virtual bool OnHandleInput(const MSG& msg) { return false; } // 場景特定的輸入處理
    
    // 輔助方法
    void SetTransparent(bool transparent) { isTransparent_ = transparent; }
    
protected:
    // 服務存取
    IServiceLocator* services_;
    IAssetManager* assetManager_;
    IUIManager* uiManager_;
    IEventManager* eventManager_;
    IConfigManager* configManager_;
    IDirect3DDevice9* device_;
    
private:
    std::string name_;
    SceneState state_;
    bool isTransparent_;
    bool initialized_;
};