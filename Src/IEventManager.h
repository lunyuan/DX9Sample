#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <mutex>

// Forward declarations
struct IServiceLocator;

// 基礎事件介面
struct IEvent {
    virtual ~IEvent() = default;
    virtual std::type_index GetTypeIndex() const = 0;
};

// 類型化事件基類
template<typename T>
struct Event : public IEvent {
    std::type_index GetTypeIndex() const override {
        return std::type_index(typeid(T));
    }
};

// 事件處理器類型
template<typename EventType>
using EventHandler = std::function<void(const EventType&)>;

// 通用事件處理器
using GenericEventHandler = std::function<void(const IEvent&)>;

// 事件管理器介面
struct IEventManager {
    virtual ~IEventManager() = default;
    
    // 註冊事件處理器
    template<typename EventType>
    void Subscribe(EventHandler<EventType> handler);
    
    // 取消註冊事件處理器
    template<typename EventType>
    void Unsubscribe();
    
    // 發送事件 (立即處理)
    template<typename EventType>
    void Publish(const EventType& event);
    
    // 排隊事件 (下次 ProcessEvents 時處理)
    template<typename EventType>
    void QueueEvent(const EventType& event);
    
    // 處理排隊的事件
    virtual void ProcessEvents() = 0;
    
    // 清空所有事件和處理器
    virtual void Clear() = 0;
    
    // 除錯資訊
    virtual size_t GetHandlerCount() const = 0;
    virtual size_t GetQueuedEventCount() const = 0;
    virtual void PrintEventInfo() const = 0;

protected:
    // 內部實作方法 (由具體類別實作)
    virtual void SubscribeInternal(std::type_index eventType, GenericEventHandler handler) = 0;
    virtual void UnsubscribeInternal(std::type_index eventType) = 0;
    virtual void PublishInternal(std::type_index eventType, const IEvent& event) = 0;
    virtual void QueueEventInternal(std::type_index eventType, std::unique_ptr<IEvent> event) = 0;
};

// 模板方法實作
template<typename EventType>
void IEventManager::Subscribe(EventHandler<EventType> handler) {
    auto genericHandler = [handler](const IEvent& event) {
        const auto& typedEvent = static_cast<const EventType&>(event);
        handler(typedEvent);
    };
    SubscribeInternal(std::type_index(typeid(EventType)), genericHandler);
}

template<typename EventType>
void IEventManager::Unsubscribe() {
    UnsubscribeInternal(std::type_index(typeid(EventType)));
}

template<typename EventType>
void IEventManager::Publish(const EventType& event) {
    PublishInternal(std::type_index(typeid(EventType)), event);
}

template<typename EventType>
void IEventManager::QueueEvent(const EventType& event) {
    auto eventCopy = std::make_unique<EventType>(event);
    QueueEventInternal(std::type_index(typeid(EventType)), std::move(eventCopy));
}

// Factory 函式
std::unique_ptr<IEventManager> CreateEventManager();

// 常用事件類型定義
namespace Events {
    // 場景事件
    struct SceneChanged : public Event<SceneChanged> {
        std::string previousSceneName;
        std::string newSceneName;
        bool isOverlay;
    };
    
    struct SceneLoaded : public Event<SceneLoaded> {
        std::string sceneName;
        bool success;
        std::string errorMessage;
    };
    
    struct SceneUnloaded : public Event<SceneUnloaded> {
        std::string sceneName;
    };
    
    // UI 事件
    struct UIComponentClicked : public Event<UIComponentClicked> {
        std::string layerName;
        std::string componentId;
        std::string componentType;
        int x, y;
        bool isRightClick;
    };
    
    struct UILayerVisibilityChanged : public Event<UILayerVisibilityChanged> {
        std::string layerName;
        bool visible;
    };
    
    struct UIFocusChanged : public Event<UIFocusChanged> {
        std::string previousLayer;
        std::string previousComponent;
        std::string newLayer;
        std::string newComponent;
    };
    
    // 資產事件
    struct AssetLoaded : public Event<AssetLoaded> {
        std::string assetPath;
        std::string assetType;
        bool success;
        std::string errorMessage;
    };
    
    struct AssetUnloaded : public Event<AssetUnloaded> {
        std::string assetPath;
        std::string assetType;
    };
    
    // 遊戲事件
    struct GameStateChanged : public Event<GameStateChanged> {
        std::string previousState;
        std::string newState;
        float transitionTime;
    };
    
    struct PlayerActionTriggered : public Event<PlayerActionTriggered> {
        std::string actionName;
        std::string playerId;
        float value;
        std::string stringValue;
    };
    
    // 系統事件
    struct WindowResized : public Event<WindowResized> {
        int oldWidth, oldHeight;
        int newWidth, newHeight;
    };
    
    struct ConfigurationChanged : public Event<ConfigurationChanged> {
        std::string configKey;
        std::string oldValue;
        std::string newValue;
    };
    
    // 除錯事件
    struct DebugCommandExecuted : public Event<DebugCommandExecuted> {
        std::string command;
        std::vector<std::string> parameters;
        bool success;
        std::string result;
    };
}