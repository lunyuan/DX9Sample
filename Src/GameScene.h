#pragma once

#include "Scene.h"
#include "ILightManager.h"
#include "ICameraController.h"
#include "IScene3D.h"
#include <memory>

// 遊戲主場景
class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() = default;

protected:
    // Scene 基類的虛擬方法實作
    bool OnInitialize() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnCleanup() override;
    
    void OnSceneEnter() override;
    void OnSceneExit() override;
    
    bool OnHandleInput(const MSG& msg) override;

private:
    // 場景特定的初始化方法
    bool InitializeAssets();
    bool InitializeUI();
    bool InitializeLighting();
    bool InitializeCamera();
    
    // UI 事件處理
    void OnPauseButtonClicked();
    void OnSettingsButtonClicked();
    void OnHelpButtonClicked();

private:
    // 遊戲場景的子系統
    std::unique_ptr<ILightManager> lightManager_;
    std::unique_ptr<ICameraController> cameraController_;
    std::unique_ptr<IScene3D> scene3D_;
    
    // 場景資產
    std::shared_ptr<struct ModelData> horseModel_;
    std::shared_ptr<struct IDirect3DTexture9> horseTexture_;
    
    // UI 元素 ID (用於管理)
    int pauseButtonId_;
    int settingsButtonId_;
    int helpButtonId_;
    int hudLayerId_;
    
    // 場景狀態
    float elapsedTime_;
    bool showDebugInfo_;
    
    // 場景設定
    int screenWidth_;
    int screenHeight_;
};