#pragma once

#include "Scene.h"
#include "EventManager.h"
#include "IEventManager.h"
#include "IUIListener.h"
#include <memory>
#include <d3d9.h>

// Forward declarations
struct IScene;
struct PauseMenuAction;
struct ModelData;

// 自定義事件示例
struct PlayerLevelUp : public Event<PlayerLevelUp> {
    std::string playerId;
    int oldLevel;
    int newLevel;
    int experienceGained;
    float timestamp;
};

struct PlayerScoreChanged : public Event<PlayerScoreChanged> {
    std::string playerId;
    int oldScore;
    int newScore;
    int scoreDelta;
    std::string reason;
};

// 遊戲主場景 - 使用新架構
class GameScene : public Scene, public EventListener, public IUIListener {
public:
    GameScene();
    ~GameScene();

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
    void CreateGameUI();
    void CreatePersistentHUD();
    
    // 事件處理
    void OnUIComponentClicked(const Events::UIComponentClicked& event);
    void OnPlayerLevelUp(const PlayerLevelUp& event);
    void OnConfigChanged(const Events::ConfigurationChanged& event);
    void OnPauseMenuAction(const PauseMenuAction& event);
    
    // IUIListener 實現
    void OnButtonClicked(UIButtonNew* button) override;
    void OnComponentClicked(UIComponentNew* component) override;
    
    // 遊戲邏輯
    void UpdateGameLogic(float deltaTime);
    void LoadGameAssets();
    void ShowLevelUpEffect(const std::string& playerId, int newLevel);
    
    // 輔助方法
    bool CheckLevelUp(const std::string& playerId, int experience);
    void TriggerScoreIncrease(int points, const std::string& reason);
    
    // UI 層級 - 已移除，使用 UIManager
    
    // UIManager 元件指標
    void* pauseButtonPtr_;
    
    // 遊戲狀態
    int playerLevel_;
    int playerExperience_;
    int score_;
    float gameTime_;
    bool isPaused_;
    std::string playerId_;
    
    // UI 元素 ID - UIManager 使用整數 ID
    int hudLayerId_;
    int gameLayerId_;
    int pauseButtonId_;
    int scoreTextId_;
    int levelTextId_;
    int expTextId_;
    
    // 3D 模型
    std::vector<std::shared_ptr<ModelData>> loadedModels_;  // 存儲所有載入的模型
    std::shared_ptr<IDirect3DTexture9> loadedTexture_; // 存儲載入的紋理
};

// Factory 函式聲明
std::unique_ptr<IScene> CreateGameScene();