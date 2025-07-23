#pragma once

#include "Scene.h"
#include "Src/EventManager.h"
#include <memory>

// 設定變更事件
struct SettingsChanged : public Event<SettingsChanged> {
    std::string settingName;
    std::string oldValue;
    std::string newValue;
    std::string category;  // "graphics", "audio", "controls", "gameplay"
};

// 設定場景 - 展示複雜的 UI 控制項和即時設定變更
class SettingsScene : public Scene, public EventListener {
public:
    SettingsScene();
    ~SettingsScene() = default;

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
    // UI 創建方法
    void CreateSettingsUI();
    void CreateGraphicsTab();
    void CreateAudioTab();
    void CreateControlsTab();
    void CreateGameplayTab();
    
    // 事件處理
    void OnUIComponentClicked(const Events::UIComponentClicked& event);
    void OnConfigChanged(const Events::ConfigurationChanged& event);
    
    // 輔助方法
    std::string GetConfigKey(const std::string& settingName);
    std::string GetSettingCategory(const std::string& settingName);
    
    // 設定管理
    void LoadCurrentSettings();
    void SaveSettings();
    void ResetToDefaults();
    void ApplySettings();
    void ResetSettings();
    void OnBackButton();
    
    // 設定更改方法
    void CycleResolution();
    void ToggleFullscreen();
    void ToggleVSync();
    void CycleAntiAliasing();
    void CycleQuality();
    
    // 標籤頁管理
    void SwitchToTab(const std::string& tabName);
    void UpdateTabVisibility();
    
    // UI 層級
    // IUILayer* settingsLayer_; // Removed - using UIManager only
    // IUILayer* currentTabLayer_; // Removed - using UIManager only
    int settingsLayerId_; // UIManager layer ID
    
    // 標籤頁系統
    std::string currentTab_;
    std::unordered_map<std::string, int> tabLayerIds_; // UIManager layer IDs
    
    // 主控制項 ID
    int backButtonId_;
    int saveButtonId_;
    int resetButtonId_;
    int applyButtonId_;
    
    // 圖形設定控制項
    int resolutionComboId_;
    int fullscreenCheckId_;
    int vsyncCheckId_;
    int antiAliasingComboId_;
    int qualityComboId_;
    int brightnessSlilerId_;
    
    // 音效設定控制項
    int masterVolumeSlilderId_;
    int musicVolumeSlilderId_;
    int sfxVolumeSlilderId_;
    int enableAudioCheckId_;
    int enable3DAudioCheckId_;
    
    // 控制設定控制項
    int mouseSensitivitySlilderId_;
    int invertMouseCheckId_;
    int keyBindingButtonsId_;
    
    // 遊戲設定控制項
    int difficultyComboId_;
    int autoSaveCheckId_;
    int showTutorialCheckId_;
    
    // 設定狀態
    bool hasUnsavedChanges_;
    std::unordered_map<std::string, std::string> currentSettings_;
    std::unordered_map<std::string, std::string> originalSettings_;
    
    // UIManager 指針（用於動態更新）
    void* fullscreenButtonPtr_;
    void* vsyncButtonPtr_;
    void* settingsContainerPtr_;  // 可拖曳的設定容器
    int brightnessTextId_;
    int graphicsLayerId_;
    
    // UI 動畫
    float tabTransitionTime_;
    float currentTransitionProgress_;
    bool isTransitioning_;
};

// Factory 函式聲明
std::unique_ptr<IScene> CreateSettingsScene();