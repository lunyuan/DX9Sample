#pragma once

#include "Scene.h"
#include "Src/EventManager.h"
#include <memory>

// 暫停場景事件
struct PauseMenuAction : public Event<PauseMenuAction> {
    std::string action;  // "resume", "settings", "quit"
    std::string sceneName;
};

// 暫停場景 - 展示透明覆蓋場景和 UI 互動
class PauseScene : public Scene, public EventListener {
public:
    PauseScene();
    ~PauseScene() = default;

protected:
    // Scene 生命週期
    bool OnInitialize() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnCleanup() override;
    void OnEnter() override;
    void OnExit() override;
    bool OnHandleInput(const MSG& msg) override;

private:
    // UI 相關
    void CreatePauseMenu();
    
    // 事件處理
    void OnUIComponentClicked(const Events::UIComponentClicked& event);
    
    // UI 層級 ID
    int pauseMenuLayerId_;
    
    // UI 元件指標 (UIManager 不返回 ID，使用指標)
    void* resumeButtonPtr_;
    void* settingsButtonPtr_;
    void* quitButtonPtr_;
    
    // 動畫相關
    float fadeInTime_;
    float currentAlpha_;
    bool isVisible_;
};

// Factory 函式聲明
std::unique_ptr<IScene> CreatePauseScene();