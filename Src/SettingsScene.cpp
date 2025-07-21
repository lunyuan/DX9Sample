#include "SettingsScene.h"
#include <iostream>
#include "PauseScene.h"
// #include "IUISystem.h" // Removed - using UIManager only
#include "IUIManager.h"
#include "IAssetManager.h"
#include "IConfigManager.h"
#include "ISceneManager.h"

SettingsScene::SettingsScene()
    : Scene("SettingsScene")
    , EventListener(nullptr)
    // , settingsLayer_(nullptr) // Removed - using UIManager only
    // , currentTabLayer_(nullptr) // Removed - using UIManager only
    , settingsLayerId_(-1)
    , currentTab_("graphics")
    , hasUnsavedChanges_(false)
    , tabTransitionTime_(0.2f)
    , currentTransitionProgress_(1.0f)
    , isTransitioning_(false)
    // Initialize all button IDs to -1
    , backButtonId_(-1)
    , saveButtonId_(-1)
    , resetButtonId_(-1)
    , applyButtonId_(-1)
    , fullscreenButtonPtr_(nullptr)
    , vsyncButtonPtr_(nullptr)
    , settingsContainerPtr_(nullptr)
    , brightnessTextId_(-1)
    , graphicsLayerId_(-1)
    , resolutionComboId_(-1)
    , fullscreenCheckId_(-1)
    , vsyncCheckId_(-1)
    , antiAliasingComboId_(-1)
    , qualityComboId_(-1)
    , brightnessSlilerId_(-1)
    , masterVolumeSlilderId_(-1)
    , musicVolumeSlilderId_(-1)
    , sfxVolumeSlilderId_(-1)
    , enableAudioCheckId_(-1)
    , enable3DAudioCheckId_(-1)
    , mouseSensitivitySlilderId_(-1)
    , invertMouseCheckId_(-1)
    , keyBindingButtonsId_(-1)
    , difficultyComboId_(-1)
    , autoSaveCheckId_(-1)
    , showTutorialCheckId_(-1)
{
    SetTransparent(false);  // 設定場景不透明，完全覆蓋背景
}

bool SettingsScene::OnInitialize() {
    if (!Scene::OnInitialize()) {
        return false;
    }
    
    // 設置 EventListener
    auto* eventManager = services_->GetEventManager();
    if (eventManager) {
        SetEventManager(eventManager);
        
        // 註冊事件處理器
        LISTEN_TO_EVENT(Events::UIComponentClicked, OnUIComponentClicked);
        LISTEN_TO_EVENT(Events::ConfigurationChanged, OnConfigChanged);
        // Note: UISliderChanged 和 UICheckboxChanged 需要在 IEventManager 中定義
        
    }
    
    // 載入當前設定
    LoadCurrentSettings();
    
    // 創建設定 UI
    CreateSettingsUI();
    
    return true;
}

void SettingsScene::OnUpdate(float deltaTime) {
    Scene::OnUpdate(deltaTime);
    
    // 處理標籤頁切換動畫
    if (isTransitioning_) {
        currentTransitionProgress_ += deltaTime / tabTransitionTime_;
        if (currentTransitionProgress_ >= 1.0f) {
            currentTransitionProgress_ = 1.0f;
            isTransitioning_ = false;
        }
        
        // 這裡可以添加動畫效果的實際實現
        // 例如：調整 UI 元素的位置、透明度等
    }
}

void SettingsScene::OnRender() {
    Scene::OnRender();
    // 設定場景的特定渲染邏輯
}

void SettingsScene::OnCleanup() {
    
    // 如果有未儲存的變更，詢問使用者
    if (hasUnsavedChanges_) {
        // 在實際實現中，這裡應該顯示確認對話框
    }
    
    // 清理 UI 層級和元素
    if (services_ && services_->GetUIManager()) {
        auto* uiManager = services_->GetUIManager();
        if (settingsLayerId_ >= 0) {
            uiManager->ClearLayer(settingsLayerId_);
            settingsLayerId_ = -1;
        }
        if (graphicsLayerId_ >= 0) {
            uiManager->ClearLayer(graphicsLayerId_);
            graphicsLayerId_ = -1;
        }
        // 清理其他標籤頁層級
        for (auto& tabPair : tabLayerIds_) {
            if (tabPair.second >= 0) {
                uiManager->ClearLayer(tabPair.second);
                tabPair.second = -1;
            }
        }
    }
    
    // 清理指標
    fullscreenButtonPtr_ = nullptr;
    vsyncButtonPtr_ = nullptr;
    settingsContainerPtr_ = nullptr;
    
    Scene::OnCleanup();
}

void SettingsScene::OnEnter() {
    Scene::OnEnter();
    
    
    // 重新載入設定以防止其他地方的變更
    LoadCurrentSettings();
    
    // 重置為圖形標籤頁
    SwitchToTab("graphics");
}

void SettingsScene::OnExit() {
    // 清理 UI 層級和元素，避免按鈕回調引用無效的場景
    if (services_ && services_->GetUIManager()) {
        auto* uiManager = services_->GetUIManager();
        if (settingsLayerId_ >= 0) {
            uiManager->ClearLayer(settingsLayerId_);
        }
        if (graphicsLayerId_ >= 0) {
            uiManager->ClearLayer(graphicsLayerId_);
        }
        // 清理其他標籤頁層級
        for (const auto& tabPair : tabLayerIds_) {
            if (tabPair.second >= 0) {
                uiManager->ClearLayer(tabPair.second);
            }
        }
    }
    
    Scene::OnExit();
    
}

bool SettingsScene::OnHandleInput(const MSG& msg) {
    // 處理快捷鍵
    if (msg.message == WM_KEYDOWN) {
        switch (msg.wParam) {
            case VK_ESCAPE:
                // ESC 鍵返回（如果有未儲存變更，應該詢問）
                if (hasUnsavedChanges_) {
                } else {
                }
                return true;
                
            case VK_F5:
                // F5 重新載入設定
                LoadCurrentSettings();
                return true;
                
            case VK_TAB:
                // Tab 鍵切換標籤頁
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    // Shift+Tab 往前切換
                } else {
                    // Tab 往後切換
                }
                return true;
        }
    }
    
    return Scene::OnHandleInput(msg);
}

void SettingsScene::CreateSettingsUI() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        std::cerr << "SettingsScene: UIManager not available" << std::endl;
        return;
    }
    
    // 創建主設定層級
    settingsLayerId_ = uiManager->CreateLayer(L"Settings", 500.0f);
    if (settingsLayerId_ < 0) {
        std::cerr << "SettingsScene: Failed to create settings layer" << std::endl;
        return;
    }
    
    // 創建可拖曳的背景容器 (使用 b-kuang.png 作為背景)
    // 位置在螢幕中央，大小適合包含所有設定內容
    int containerWidth = 600;
    int containerHeight = 700;
    int containerX = 100;  // 可以調整為螢幕中央
    int containerY = 50;
    
    auto* settingsContainer = uiManager->CreateImage(L"b-kuang.png", containerX, containerY, containerWidth, containerHeight, 
                                                    true, nullptr, false);  // draggable=true, parent=nullptr, allowDragFromTransparent=false
    
    // 創建標題 (作為容器的子元素，使用相對座標)
    uiManager->AddText(L"GAME SETTINGS", containerX + 200, containerY + 20, 300, 40, 0xFFFFFFFF, settingsLayerId_);
    
    // 創建標籤頁按鈕 (作為容器的子元素)
    auto* graphicsBtn = uiManager->CreateButton(L"Graphics", 20, 70, 120, 30, 
        [this]() { SwitchToTab("graphics"); }, settingsContainer);
    auto* audioBtn = uiManager->CreateButton(L"Audio", 160, 70, 120, 30,
        [this]() { SwitchToTab("audio"); }, settingsContainer);
    auto* controlsBtn = uiManager->CreateButton(L"Controls", 300, 70, 120, 30,
        [this]() { SwitchToTab("controls"); }, settingsContainer);
    auto* gameplayBtn = uiManager->CreateButton(L"Gameplay", 440, 70, 120, 30,
        [this]() { SwitchToTab("gameplay"); }, settingsContainer);
    
    // 創建主控制按鈕 (作為容器的子元素)
    auto* backBtn = uiManager->CreateButton(L"Back", 20, 620, 100, 40,
        [this]() { OnBackButton(); }, settingsContainer);
    auto* saveBtn = uiManager->CreateButton(L"Save", 140, 620, 100, 40,
        [this]() { SaveSettings(); }, settingsContainer);
    auto* applyBtn = uiManager->CreateButton(L"Apply", 260, 620, 100, 40,
        [this]() { ApplySettings(); }, settingsContainer);
    auto* resetBtn = uiManager->CreateButton(L"Reset", 380, 620, 100, 40,
        [this]() { ResetSettings(); }, settingsContainer);
    
    // 保存容器指標以便後續使用
    settingsContainerPtr_ = settingsContainer;
    
    // 創建各個標籤頁
    CreateGraphicsTab();
    CreateAudioTab();
    CreateControlsTab();
    CreateGameplayTab();
    
    // 初始顯示圖形標籤頁
    SwitchToTab("graphics");
}

void SettingsScene::CreateGraphicsTab() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager || !settingsContainerPtr_) return;
    
    // 創建圖形設定標籤頁層級
    graphicsLayerId_ = uiManager->CreateLayer(L"GraphicsTab", 510.0f);
    
    // 轉型為 UIComponentNew 以便作為父元素使用
    auto* container = static_cast<UIComponentNew*>(settingsContainerPtr_);
    
    // 解析度設定 (相對於容器的座標)
    // 注意：AddText 不支援父子關係，所以文字需要使用絕對座標
    uiManager->AddText(L"Resolution:", 150, 200, 100, 25, 0xFFFFFFFF, graphicsLayerId_);
    auto* resolutionBtn = uiManager->CreateButton(L"1280x720", 170, 150, 200, 25,
        [this]() { CycleResolution(); }, container);
    
    // 全螢幕設定
    auto* fullscreenBtn = uiManager->CreateButton(L"Fullscreen: OFF", 50, 190, 200, 25,
        [this]() { ToggleFullscreen(); }, container);
    fullscreenButtonPtr_ = fullscreenBtn;
    
    // 垂直同步
    auto* vsyncBtn = uiManager->CreateButton(L"V-Sync: ON", 50, 230, 200, 25,
        [this]() { ToggleVSync(); }, container);
    vsyncButtonPtr_ = vsyncBtn;
    
    // 反鋸齒
    uiManager->AddText(L"Anti-Aliasing:", 150, 320, 100, 25, 0xFFFFFFFF, graphicsLayerId_);
    auto* antiAliasingBtn = uiManager->CreateButton(L"Off", 170, 270, 200, 25,
        [this]() { CycleAntiAliasing(); }, container);
    
    // 品質設定
    uiManager->AddText(L"Quality:", 150, 360, 100, 25, 0xFFFFFFFF, graphicsLayerId_);
    auto* qualityBtn = uiManager->CreateButton(L"Medium", 170, 310, 200, 25,
        [this]() { CycleQuality(); }, container);
    
    // 亮度設定
    uiManager->AddText(L"Brightness:", 150, 400, 100, 25, 0xFFFFFFFF, graphicsLayerId_);
    brightnessTextId_ = uiManager->AddText(L"50%", 270, 400, 200, 25, 0xFFFFFFFF, graphicsLayerId_);
}

void SettingsScene::CreateAudioTab() {
    // TODO: Convert to UIManager
    return;
    
    // 創建音效設定標籤頁層級
    // auto* audioLayer = uiSystem->CreateLayer("AudioTab", UILayerType::Interface, 510);
    // tabLayers_["audio"] = audioLayer;
    
    return; // TODO: Convert all below to UIManager
    /*
    // 主音量
    audioLayer->CreateText("Master Volume:", 150, 200, 120, 25, 0xFFFFFFFF);
    masterVolumeSlilderId_ = audioLayer->CreateButton("80%", 280, 200, 200, 25);
    
    // 音樂音量
    audioLayer->CreateText("Music Volume:", 150, 240, 120, 25, 0xFFFFFFFF);
    musicVolumeSlilderId_ = audioLayer->CreateButton("70%", 280, 240, 200, 25);
    
    // 音效音量
    audioLayer->CreateText("SFX Volume:", 150, 280, 120, 25, 0xFFFFFFFF);
    sfxVolumeSlilderId_ = audioLayer->CreateButton("90%", 280, 280, 200, 25);
    
    // 啟用音效
    enableAudioCheckId_ = audioLayer->CreateButton("Enable Audio: ON", 150, 320, 200, 25);
    
    // 3D 音效
    enable3DAudioCheckId_ = audioLayer->CreateButton("3D Audio: ON", 150, 360, 200, 25);
    
    // 設定為不可見（初始狀態）
    audioLayer->SetVisible(false);
    */
}

void SettingsScene::CreateControlsTab() {
    // TODO: Convert to UIManager
    return;
    
    // 創建控制設定標籤頁層級
    // auto* controlsLayer = uiSystem->CreateLayer("ControlsTab", UILayerType::Interface, 510);
    // tabLayers_["controls"] = controlsLayer;
    
    return; // TODO: Convert all below to UIManager
    /*
    // 滑鼠靈敏度
    controlsLayer->CreateText("Mouse Sensitivity:", 150, 200, 150, 25, 0xFFFFFFFF);
    mouseSensitivitySlilderId_ = controlsLayer->CreateButton("1.0x", 310, 200, 200, 25);
    
    // 反轉滑鼠
    invertMouseCheckId_ = controlsLayer->CreateButton("Invert Mouse: OFF", 150, 240, 200, 25);
    
    // 按鍵綁定區域
    controlsLayer->CreateText("Key Bindings:", 150, 280, 150, 25, 0xFFFFFFFF);
    controlsLayer->CreateText("Move Forward: W", 150, 310, 200, 20, 0xFFCCCCCC);
    controlsLayer->CreateText("Move Backward: S", 150, 330, 200, 20, 0xFFCCCCCC);
    controlsLayer->CreateText("Move Left: A", 150, 350, 200, 20, 0xFFCCCCCC);
    controlsLayer->CreateText("Move Right: D", 150, 370, 200, 20, 0xFFCCCCCC);
    controlsLayer->CreateText("Jump: Space", 150, 390, 200, 20, 0xFFCCCCCC);
    controlsLayer->CreateText("Run: Shift", 150, 410, 200, 20, 0xFFCCCCCC);
    
    // 重設按鍵按鈕
    controlsLayer->CreateButton("Reset to Defaults", 350, 350, 150, 30);
    
    // 設定為不可見（初始狀態）
    controlsLayer->SetVisible(false);
    */
}

void SettingsScene::CreateGameplayTab() {
    // TODO: Convert to UIManager  
    return;
    
    // 創建遊戲設定標籤頁層級
    // auto* gameplayLayer = uiSystem->CreateLayer("GameplayTab", UILayerType::Interface, 510);
    // tabLayers_["gameplay"] = gameplayLayer;
    
    return; // TODO: Convert all below to UIManager
    /*
    // 難度設定
    gameplayLayer->CreateText("Difficulty:", 150, 200, 100, 25, 0xFFFFFFFF);
    difficultyComboId_ = gameplayLayer->CreateButton("Normal", 270, 200, 200, 25);
    
    // 自動儲存
    autoSaveCheckId_ = gameplayLayer->CreateButton("Auto Save: ON", 150, 240, 200, 25);
    
    // 顯示教學
    showTutorialCheckId_ = gameplayLayer->CreateButton("Tutorial: ON", 150, 280, 200, 25);
    
    // 遊戲統計
    gameplayLayer->CreateText("Game Statistics:", 150, 320, 150, 25, 0xFFFFFFFF);
    gameplayLayer->CreateText("Playtime: 12h 34m", 150, 350, 200, 20, 0xFFCCCCCC);
    gameplayLayer->CreateText("Games Played: 42", 150, 370, 200, 20, 0xFFCCCCCC);
    gameplayLayer->CreateText("High Score: 9999", 150, 390, 200, 20, 0xFFCCCCCC);
    
    // 重設統計按鈕
    gameplayLayer->CreateButton("Reset Statistics", 350, 370, 150, 30);
    
    // 設定為不可見（初始狀態）
    gameplayLayer->SetVisible(false);
    */
}

void SettingsScene::OnUIComponentClicked(const Events::UIComponentClicked& event) {
    
    // 處理標籤頁按鈕
    // UIManager 使用回調函式處理點擊，所以這個方法可能不會被調用
    // 保留此函式以相容事件系統
    
    // 處理主控制按鈕
    // TODO: Convert event.componentId to int for comparison
    // if (std::stoi(event.componentId) == backButtonId_) {
    if (false) { // Temporary to avoid compilation errors
        if (hasUnsavedChanges_) {
            // 在實際實現中，這裡應該顯示確認對話框
            // 暫時直接發送返回事件
        }
        
        // 發送場景切換事件返回上一個場景
        PauseMenuAction backAction;
        backAction.action = "back_to_pause";
        backAction.sceneName = "PauseScene";
        Emit(backAction);
        
        
    // } else if (std::stoi(event.componentId) == saveButtonId_) {
    //     SaveSettings();
    //     
    // } else if (std::stoi(event.componentId) == applyButtonId_) {
    //     ApplySettings();
    //     
    // } else if (std::stoi(event.componentId) == resetButtonId_) {
    //     ResetToDefaults();
    }
}

void SettingsScene::OnConfigChanged(const Events::ConfigurationChanged& event) {
    
    // 標記有未儲存的變更
    hasUnsavedChanges_ = true;
}

void SettingsScene::LoadCurrentSettings() {
    auto* configManager = services_->GetConfigManager();
    if (!configManager) return;
    
    // 載入並儲存當前設定值
    originalSettings_["resolution"] = configManager->GetString("graphics.resolution", "1280x720");
    originalSettings_["fullscreen"] = configManager->GetBool("graphics.fullscreen", false) ? "true" : "false";
    originalSettings_["vsync"] = configManager->GetBool("graphics.vsync", true) ? "true" : "false";
    originalSettings_["antialiasing"] = configManager->GetString("graphics.antialiasing", "Off");
    originalSettings_["quality"] = configManager->GetString("graphics.quality", "Medium");
    originalSettings_["brightness"] = std::to_string(configManager->GetFloat("graphics.brightness", 50.0f));
    
    originalSettings_["master_volume"] = std::to_string(configManager->GetFloat("audio.master_volume", 80.0f));
    originalSettings_["music_volume"] = std::to_string(configManager->GetFloat("audio.music_volume", 70.0f));
    originalSettings_["sfx_volume"] = std::to_string(configManager->GetFloat("audio.sfx_volume", 90.0f));
    originalSettings_["enable_audio"] = configManager->GetBool("audio.enabled", true) ? "true" : "false";
    originalSettings_["enable_3d_audio"] = configManager->GetBool("audio.3d_enabled", true) ? "true" : "false";
    
    originalSettings_["mouse_sensitivity"] = std::to_string(configManager->GetFloat("controls.mouse_sensitivity", 1.0f));
    originalSettings_["invert_mouse"] = configManager->GetBool("controls.invert_mouse", false) ? "true" : "false";
    
    originalSettings_["difficulty"] = configManager->GetString("gameplay.difficulty", "Normal");
    originalSettings_["auto_save"] = configManager->GetBool("gameplay.auto_save", true) ? "true" : "false";
    originalSettings_["show_tutorial"] = configManager->GetBool("gameplay.show_tutorial", true) ? "true" : "false";
    
    // 複製到當前設定
    currentSettings_ = originalSettings_;
    hasUnsavedChanges_ = false;
    
}

void SettingsScene::SaveSettings() {
    auto* configManager = services_->GetConfigManager();
    if (!configManager) return;
    
    // 儲存所有設定到配置管理器
    for (const auto& setting : currentSettings_) {
        const std::string& key = setting.first;
        const std::string& value = setting.second;
        
        // 根據設定類型決定如何儲存
        if (key.find("volume") != std::string::npos || key.find("brightness") != std::string::npos || 
            key.find("sensitivity") != std::string::npos) {
            configManager->SetFloat(GetConfigKey(key), std::stof(value));
        } else if (key.find("enable") != std::string::npos || key.find("fullscreen") != std::string::npos ||
                   key.find("vsync") != std::string::npos || key.find("auto_save") != std::string::npos ||
                   key.find("show_tutorial") != std::string::npos || key.find("invert_mouse") != std::string::npos) {
            configManager->SetBool(GetConfigKey(key), value == "true");
        } else {
            configManager->SetString(GetConfigKey(key), value);
        }
    }
    
    // 儲存配置檔案
    configManager->SaveConfig("config/settings.json");
    
    // 更新原始設定並清除變更標記
    originalSettings_ = currentSettings_;
    hasUnsavedChanges_ = false;
    
    // 發送設定變更事件
    SettingsChanged settingsEvent;
    settingsEvent.settingName = "all";
    settingsEvent.oldValue = "various";
    settingsEvent.newValue = "saved";
    settingsEvent.category = "system";
    Emit(settingsEvent);
}

void SettingsScene::ResetToDefaults() {
    // 重設所有設定為預設值
    currentSettings_["resolution"] = "1280x720";
    currentSettings_["fullscreen"] = "false";
    currentSettings_["vsync"] = "true";
    currentSettings_["antialiasing"] = "Off";
    currentSettings_["quality"] = "Medium";
    currentSettings_["brightness"] = "50.0";
    
    currentSettings_["master_volume"] = "80.0";
    currentSettings_["music_volume"] = "70.0";
    currentSettings_["sfx_volume"] = "90.0";
    currentSettings_["enable_audio"] = "true";
    currentSettings_["enable_3d_audio"] = "true";
    
    currentSettings_["mouse_sensitivity"] = "1.0";
    currentSettings_["invert_mouse"] = "false";
    
    currentSettings_["difficulty"] = "Normal";
    currentSettings_["auto_save"] = "true";
    currentSettings_["show_tutorial"] = "true";
    
    hasUnsavedChanges_ = true;
    
    // 這裡應該更新 UI 控制項的值
    // UpdateUIFromSettings();
}

void SettingsScene::ApplySettings() {
    auto* configManager = services_->GetConfigManager();
    if (!configManager) return;
    
    // 立即套用設定而不儲存到檔案
    for (const auto& setting : currentSettings_) {
        const std::string& key = setting.first;
        const std::string& value = setting.second;
        
        // 發送個別設定變更事件
        SettingsChanged settingsEvent;
        settingsEvent.settingName = key;
        settingsEvent.oldValue = originalSettings_[key];
        settingsEvent.newValue = value;
        settingsEvent.category = GetSettingCategory(key);
        Emit(settingsEvent);
    }
    
}

void SettingsScene::SwitchToTab(const std::string& tabName) {
    if (tabName == currentTab_) return;
    
    
    // 隱藏當前標籤頁
    // TODO: Convert to UIManager layer visibility
    // if (currentTabLayer_) {
    //     currentTabLayer_->SetVisible(false);
    // }
    
    // 顯示新標籤頁
    auto it = tabLayerIds_.find(tabName);
    if (it != tabLayerIds_.end()) {
        // TODO: Set layer visibility using UIManager
        // currentTabLayer_ = it->second;
        // currentTabLayer_->SetVisible(true);
        currentTab_ = tabName;
        
        // 啟動切換動畫
        isTransitioning_ = true;
        currentTransitionProgress_ = 0.0f;
    }
}

void SettingsScene::UpdateTabVisibility() {
    // 確保只有當前標籤頁可見
    // TODO: Convert to UIManager layer visibility
    // for (const auto& tabPair : tabLayerIds_) {
    //     bool shouldBeVisible = (tabPair.first == currentTab_);
    //     // UIManager needs SetLayerVisible method
    // }
}

std::string SettingsScene::GetConfigKey(const std::string& settingName) {
    // 將設定名稱轉換為配置鍵
    if (settingName.find("graphics") == 0 || settingName == "resolution" || 
        settingName == "fullscreen" || settingName == "vsync" || 
        settingName == "antialiasing" || settingName == "quality" || settingName == "brightness") {
        return "graphics." + settingName;
    } else if (settingName.find("audio") == 0 || settingName.find("volume") != std::string::npos ||
               settingName.find("enable") != std::string::npos) {
        return "audio." + settingName;
    } else if (settingName.find("controls") == 0 || settingName.find("mouse") != std::string::npos) {
        return "controls." + settingName;
    } else if (settingName.find("gameplay") == 0 || settingName == "difficulty" ||
               settingName == "auto_save" || settingName == "show_tutorial") {
        return "gameplay." + settingName;
    }
    return settingName;
}

std::string SettingsScene::GetSettingCategory(const std::string& settingName) {
    if (settingName.find("graphics") == 0 || settingName == "resolution" || 
        settingName == "fullscreen" || settingName == "vsync") {
        return "graphics";
    } else if (settingName.find("audio") == 0 || settingName.find("volume") != std::string::npos) {
        return "audio";
    } else if (settingName.find("controls") == 0 || settingName.find("mouse") != std::string::npos) {
        return "controls";
    } else if (settingName.find("gameplay") == 0 || settingName == "difficulty") {
        return "gameplay";
    }
    return "unknown";
}

void SettingsScene::OnBackButton() {
    // 返回到上一個場景
    if (services_ && services_->GetSceneManager()) {
        services_->GetSceneManager()->PopScene();
    }
}

void SettingsScene::ResetSettings() {
    // 重置到預設值
    LoadCurrentSettings();
    hasUnsavedChanges_ = false;
}

void SettingsScene::CycleResolution() {
    // 循環切換解析度
    // TODO: 實現解析度切換邏輯
}

void SettingsScene::ToggleFullscreen() {
    // 切換全螢幕狀態
    bool isFullscreen = currentSettings_["graphics.fullscreen"] == "true";
    currentSettings_["graphics.fullscreen"] = isFullscreen ? "false" : "true";
    
    // 更新按鈕文字
    if (fullscreenButtonPtr_ && uiManager_) {
        std::wstring text = isFullscreen ? L"Fullscreen: OFF" : L"Fullscreen: ON";
        // UIManager 需要更新按鈕文字的功能
    }
    
    hasUnsavedChanges_ = true;
}

void SettingsScene::ToggleVSync() {
    // 切換垂直同步
    bool vsyncEnabled = currentSettings_["graphics.vsync"] == "true";
    currentSettings_["graphics.vsync"] = vsyncEnabled ? "false" : "true";
    
    // 更新按鈕文字
    if (vsyncButtonPtr_ && uiManager_) {
        std::wstring text = vsyncEnabled ? L"V-Sync: OFF" : L"V-Sync: ON";
        // UIManager 需要更新按鈕文字的功能
    }
    
    hasUnsavedChanges_ = true;
}

void SettingsScene::CycleAntiAliasing() {
    // 循環切換反鋸齒設定
    // TODO: 實現反鋸齒切換邏輯
}

void SettingsScene::CycleQuality() {
    // 循環切換品質設定
    // TODO: 實現品質切換邏輯
}

// Factory 函式
std::unique_ptr<IScene> CreateSettingsScene() {
    return std::make_unique<SettingsScene>();
}