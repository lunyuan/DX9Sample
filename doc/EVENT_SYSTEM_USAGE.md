# 事件系統使用指南

## 概述

新的 IEventManager 系統提供了類型安全的事件處理架構，支援立即發布和排隊處理，解決了組件間的耦合問題，實現了現代遊戲引擎的事件驅動架構。

## 核心概念

### 1. 類型安全事件

事件系統使用模板和 RTTI 確保類型安全：

```cpp
// 定義自定義事件
struct PlayerLevelUp : public Event<PlayerLevelUp> {
    std::string playerId;
    int oldLevel;
    int newLevel;
    int experienceGained;
};

// 註冊事件處理器
eventManager->Subscribe<PlayerLevelUp>([](const PlayerLevelUp& event) {
    std::cout << "Player " << event.playerId 
              << " leveled up from " << event.oldLevel 
              << " to " << event.newLevel << std::endl;
});

// 發送事件
eventManager->Publish(PlayerLevelUp{
    .playerId = "player_001",
    .oldLevel = 5,
    .newLevel = 6,
    .experienceGained = 1000
});
```

### 2. 立即發布 vs 排隊處理

```cpp
// 立即發布 - 同步處理
eventManager->Publish(event);  // 立即調用所有處理器

// 排隊處理 - 異步處理
eventManager->QueueEvent(event);  // 添加到佇列
eventManager->ProcessEvents();     // 在適當時機批次處理
```

### 3. EventListener 基類

便利的基類自動管理事件訂閱：

```cpp
class GameScene : public Scene, public EventListener {
public:
    GameScene(IEventManager* eventManager) 
        : EventListener(eventManager) {
        
        // 使用便利巨集註冊事件
        LISTEN_TO_EVENT(PlayerLevelUp, OnPlayerLevelUp);
        LISTEN_TO_EVENT(UIComponentClicked, OnUIClicked);
    }
    
private:
    void OnPlayerLevelUp(const PlayerLevelUp& event) {
        // 處理玩家升級事件
        ShowLevelUpEffect(event.playerId, event.newLevel);
    }
    
    void OnUIClicked(const UIComponentClicked& event) {
        // 處理 UI 點擊事件
        if (event.componentId == "pause_button") {
            EMIT_EVENT(GameStateChanged, "playing", "paused", 0.5f);
        }
    }
};
```

## 解決的問題

### 🚫 原來的問題

```cpp
// 緊耦合的組件通信
class UIManager {
    GameScene* gameScene_;  // 直接依賴場景
    
public:
    void OnButtonClicked(const std::string& buttonId) {
        if (buttonId == "pause") {
            gameScene_->Pause();  // 直接調用，緊耦合
        }
    }
};

class GameScene {
    UIManager* uiManager_;  // 循環依賴
    PlayerManager* playerManager_;
    
public:
    void OnPlayerLevelUp(int playerId, int newLevel) {
        // 需要通知多個系統
        uiManager_->UpdateLevelDisplay(newLevel);  // 直接調用
        SoundManager::PlayLevelUpSound();          // 全域狀態
        // ... 更多直接調用
    }
};
```

**問題**：
1. 組件間緊耦合，難以測試和維護
2. 循環依賴問題
3. 難以擴展新的事件監聽器
4. 無法控制事件處理順序

### ✅ 新的解決方案

```cpp
// 鬆耦合的事件驅動架構
class UIManager : public EventListener {
public:
    UIManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(Events::UIComponentClicked, OnComponentClicked);
        LISTEN_TO_EVENT(Events::GameStateChanged, OnGameStateChanged);
        LISTEN_TO_EVENT(PlayerLevelUp, OnPlayerLevelUp);
    }
    
private:
    void OnComponentClicked(const Events::UIComponentClicked& event) {
        if (event.componentId == "pause_button") {
            EMIT_EVENT(Events::GameStateChanged, "playing", "paused", 0.5f);
        }
    }
    
    void OnPlayerLevelUp(const PlayerLevelUp& event) {
        UpdateLevelDisplay(event.newLevel);
    }
};

class GameScene : public Scene, public EventListener {
public:
    void OnPlayerExperienceGained(int playerId, int exp) {
        if (CheckLevelUp(playerId, exp)) {
            EMIT_EVENT(PlayerLevelUp, playerId, oldLevel, newLevel, exp);
        }
    }
};

class SoundManager : public EventListener {
public:
    SoundManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(PlayerLevelUp, OnPlayerLevelUp);
        LISTEN_TO_EVENT(Events::UIComponentClicked, OnUISound);
    }
    
private:
    void OnPlayerLevelUp(const PlayerLevelUp& event) {
        PlaySound("level_up.wav");
    }
};
```

## 基本使用方式

### 1. 初始化事件系統

```cpp
// 在 EngineContext 中初始化
class EngineContext {
private:
    std::unique_ptr<IEventManager> eventManager_;
    
public:
    STDMETHODIMP Initialize(HWND hwnd, UINT width, UINT height) {
        // 創建事件管理器
        eventManager_ = CreateEventManager();
        
        // 設置到服務定位器
        serviceLocator_->SetEventManager(eventManager_.get());
        
        // 初始化其他系統...
        
        return S_OK;
    }
    
    STDMETHODIMP Run() {
        while (running) {
            // 處理排隊的事件
            eventManager_->ProcessEvents();
            
            // 更新和渲染...
        }
    }
};
```

### 2. 在場景中使用事件

```cpp
class MainMenuScene : public Scene, public EventListener {
protected:
    bool OnInitialize() override {
        auto* eventManager = services_->GetEventManager();
        
        // 設置 EventListener
        eventManager_ = eventManager;  // 保存引用給基類
        
        // 註冊事件處理器
        LISTEN_TO_EVENT(Events::UIComponentClicked, OnUIClicked);
        LISTEN_TO_EVENT(Events::ConfigurationChanged, OnConfigChanged);
        
        CreateMenuUI();
        return true;
    }
    
    void CreateMenuUI() {
        auto* uiSystem = services_->GetUISystem();
        auto* menuLayer = uiSystem->CreateLayer("MainMenu", UILayerType::Interface);
        
        // 創建開始遊戲按鈕
        menuLayer->CreateButton("開始遊戲", 300, 200, 200, 50,
            [this](const UIEvent& uiEvent) {
                // 發送 UI 點擊事件
                EMIT_EVENT(Events::UIComponentClicked,
                    "MainMenu", uiEvent.componentId, "button", 
                    uiEvent.x, uiEvent.y, false);
            });
    }
    
private:
    void OnUIClicked(const Events::UIComponentClicked& event) {
        if (event.componentId.find("start_game") != std::string::npos) {
            // 發送場景切換事件
            EMIT_EVENT(Events::SceneChanged, "MainMenu", "GameScene", false);
        }
    }
    
    void OnConfigChanged(const Events::ConfigurationChanged& event) {
        if (event.configKey == "language") {
            RefreshMenuText();
        }
    }
};
```

### 3. 場景管理器整合

```cpp
class SceneManager : public EventListener {
public:
    SceneManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(Events::SceneChanged, OnSceneChangeRequested);
    }
    
private:
    void OnSceneChangeRequested(const Events::SceneChanged& event) {
        if (event.isOverlay) {
            PushScene(event.newSceneName);
        } else {
            SwitchToScene(event.newSceneName);
        }
        
        // 通知場景載入完成
        EMIT_EVENT(Events::SceneLoaded, event.newSceneName, true, "");
    }
    
    void SwitchToScene(const std::string& sceneName) {
        if (currentScene_) {
            currentScene_->OnExit();
            EMIT_EVENT(Events::SceneUnloaded, currentScene_->GetName());
        }
        
        currentScene_ = CreateScene(sceneName);
        if (currentScene_) {
            currentScene_->Initialize(services_);
            currentScene_->OnEnter();
        }
    }
};
```

## 進階功能

### 1. 自定義事件類型

```cpp
// 定義遊戲特定事件
namespace GameEvents {
    struct InventoryItemAdded : public Event<InventoryItemAdded> {
        std::string playerId;
        std::string itemId;
        int quantity;
        std::string rarity;
    };
    
    struct QuestCompleted : public Event<QuestCompleted> {
        std::string questId;
        std::string playerId;
        int rewardExp;
        std::vector<std::string> rewardItems;
    };
    
    struct CombatEvent : public Event<CombatEvent> {
        enum Type { Attack, Block, Dodge, Critical } type;
        std::string attackerId;
        std::string targetId;
        float damage;
        bool isPlayer;
    };
}

// 使用自定義事件
class InventoryManager : public EventListener {
public:
    InventoryManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(GameEvents::InventoryItemAdded, OnItemAdded);
        LISTEN_TO_EVENT(GameEvents::QuestCompleted, OnQuestCompleted);
    }
    
private:
    void OnItemAdded(const GameEvents::InventoryItemAdded& event) {
        AddItemToInventory(event.itemId, event.quantity);
        
        // 檢查是否為稀有物品
        if (event.rarity == "legendary") {
            EMIT_EVENT(Events::UILayerVisibilityChanged, "RareItemNotification", true);
        }
    }
    
    void OnQuestCompleted(const GameEvents::QuestCompleted& event) {
        // 給予獎勵物品
        for (const auto& itemId : event.rewardItems) {
            EMIT_EVENT(GameEvents::InventoryItemAdded,
                event.playerId, itemId, 1, "common");
        }
    }
};
```

### 2. 異步事件處理

```cpp
class AssetLoadingManager : public EventListener {
public:
    AssetLoadingManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(Events::SceneLoaded, OnSceneLoaded);
    }
    
private:
    void OnSceneLoaded(const Events::SceneLoaded& event) {
        // 異步載入場景資產
        LoadSceneAssetsAsync(event.sceneName, [this, sceneName = event.sceneName](bool success) {
            // 在主線程中排隊事件
            EMIT_QUEUED_EVENT(Events::AssetLoaded, 
                "scene:" + sceneName, "scene", success, 
                success ? "" : "Failed to load scene assets");
        });
    }
    
    void LoadSceneAssetsAsync(const std::string& sceneName, std::function<void(bool)> callback) {
        // 在背景線程載入資產
        std::thread([sceneName, callback]() {
            bool success = LoadAssetsFromFile("scenes/" + sceneName + ".assets");
            callback(success);
        }).detach();
    }
};
```

### 3. 事件過濾和條件處理

```cpp
class AchievementManager : public EventListener {
public:
    AchievementManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(PlayerLevelUp, OnPlayerLevelUp);
        LISTEN_TO_EVENT(GameEvents::CombatEvent, OnCombatEvent);
        LISTEN_TO_EVENT(GameEvents::QuestCompleted, OnQuestCompleted);
    }
    
private:
    void OnPlayerLevelUp(const PlayerLevelUp& event) {
        // 檢查等級相關成就
        if (event.newLevel == 10) {
            UnlockAchievement("first_10_levels");
        } else if (event.newLevel == 50) {
            UnlockAchievement("veteran_player");
        } else if (event.newLevel >= 100) {
            UnlockAchievement("master_level");
        }
    }
    
    void OnCombatEvent(const GameEvents::CombatEvent& event) {
        if (event.type == GameEvents::CombatEvent::Critical && event.isPlayer) {
            criticalHitCount_++;
            
            if (criticalHitCount_ >= 100) {
                UnlockAchievement("critical_master");
            }
        }
    }
    
    void UnlockAchievement(const std::string& achievementId) {
        // 發送成就解鎖事件
        EMIT_EVENT(AchievementUnlocked, achievementId, GetCurrentPlayerId());
        
        // 顯示成就通知 UI
        EMIT_EVENT(Events::UILayerVisibilityChanged, "AchievementNotification", true);
    }

private:
    int criticalHitCount_ = 0;
};
```

### 4. 事件系統除錯

```cpp
class DebugManager : public EventListener {
public:
    DebugManager(IEventManager* eventManager) : EventListener(eventManager) {
        eventManager->SetDebugMode(true);  // 啟用除錯模式
        
        // 監聽所有類型的事件進行日誌記錄
        LISTEN_TO_EVENT(Events::SceneChanged, LogEvent);
        LISTEN_TO_EVENT(Events::UIComponentClicked, LogEvent);
        LISTEN_TO_EVENT(PlayerLevelUp, LogEvent);
    }
    
    void PrintSystemStatus() {
        auto* eventManager = GetEventManager();
        
        std::cout << "\n=== Event System Status ===" << std::endl;
        eventManager->PrintEventInfo();
        
        std::cout << "Recent Events (last 10):" << std::endl;
        for (const auto& logEntry : recentEvents_) {
            std::cout << "  " << logEntry << std::endl;
        }
        std::cout << "============================\n" << std::endl;
    }
    
private:
    template<typename EventType>
    void LogEvent(const EventType& event) {
        std::string eventLog = "Event: " + typeid(EventType).name() + " at " + GetCurrentTime();
        recentEvents_.push_back(eventLog);
        
        if (recentEvents_.size() > 10) {
            recentEvents_.erase(recentEvents_.begin());
        }
    }
    
    std::vector<std::string> recentEvents_;
};
```

## 與其他系統整合

### 1. UI 系統整合

```cpp
// 更新 UISystem 以發送事件
class UISystem : public IUISystem {
public:
    UISystem(IEventManager* eventManager) : eventManager_(eventManager) {}
    
    void OnComponentClicked(const std::string& layerName, const std::string& componentId,
                           int x, int y, bool isRightClick) {
        // 發送 UI 點擊事件
        Events::UIComponentClicked event;
        event.layerName = layerName;
        event.componentId = componentId;
        event.x = x;
        event.y = y;
        event.isRightClick = isRightClick;
        
        eventManager_->Publish(event);
    }

private:
    IEventManager* eventManager_;
};
```

### 2. 資產管理器整合

```cpp
class AssetManager : public IAssetManager, public EventListener {
public:
    AssetManager(IEventManager* eventManager) : EventListener(eventManager) {
        LISTEN_TO_EVENT(Events::SceneChanged, OnSceneChanged);
    }
    
    std::shared_ptr<ModelData> LoadModel(const std::string& assetPath) override {
        auto model = LoadModelInternal(assetPath);
        
        // 發送資產載入事件
        EMIT_EVENT(Events::AssetLoaded, assetPath, "model", 
                   model != nullptr, model ? "" : "Failed to load model");
        
        return model;
    }
    
private:
    void OnSceneChanged(const Events::SceneChanged& event) {
        // 預載入新場景的資產
        PreloadSceneAssets(event.newSceneName);
    }
};
```

## 優勢總結

1. **🔗 鬆耦合**: 組件間通過事件通信，無直接依賴
2. **🛡️ 類型安全**: 編譯時檢查事件類型，避免運行時錯誤
3. **⚡ 性能**: 支援異步處理，避免阻塞主線程
4. **🧩 擴展性**: 輕鬆添加新的事件監聽器和事件類型
5. **🐛 除錯友善**: 豐富的日誌和統計功能
6. **📚 現代化**: 遵循現代遊戲引擎架構模式

這個事件系統完全解決了組件間緊耦合的問題，為您的引擎提供了靈活、強大的事件驅動架構！