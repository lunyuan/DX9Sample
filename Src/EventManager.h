#pragma once

#include "IEventManager.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>

// 排隊事件的包裝
struct QueuedEvent {
    std::type_index type;
    std::unique_ptr<IEvent> event;
    
    QueuedEvent(std::type_index t, std::unique_ptr<IEvent> e)
        : type(t), event(std::move(e)) {}
};

// Forward declaration
class EventListener;

class EventManager : public IEventManager {
    friend class EventListener; // Allow EventListener to access protected members
    
public:
    EventManager();
    ~EventManager();
    
    // IEventManager 介面實作
    void ProcessEvents() override;
    void Clear() override;
    
    // 除錯
    size_t GetHandlerCount() const override;
    size_t GetQueuedEventCount() const override;
    void PrintEventInfo() const override;
    
    // 設定
    void SetDebugMode(bool enabled) { debugMode_ = enabled; }
    bool IsDebugMode() const { return debugMode_; }
    
    // 統計
    size_t GetProcessedEventCount() const { return processedEventCount_; }
    size_t GetPublishedEventCount() const { return publishedEventCount_; }
    void ResetStatistics();

protected:
    // IEventManager 內部實作
    void SubscribeInternal(std::type_index eventType, GenericEventHandler handler) override;
    void UnsubscribeInternal(std::type_index eventType) override;
    void PublishInternal(std::type_index eventType, const IEvent& event) override;
    void QueueEventInternal(std::type_index eventType, std::unique_ptr<IEvent> event) override;

private:
    // 事件處理器儲存
    std::unordered_map<std::type_index, std::vector<GenericEventHandler>> eventHandlers_;
    mutable std::shared_mutex handlerMutex_;
    
    // 事件佇列
    std::queue<QueuedEvent> eventQueue_;
    mutable std::mutex queueMutex_;
    
    // 統計和除錯
    std::atomic<size_t> processedEventCount_;
    std::atomic<size_t> publishedEventCount_;
    bool debugMode_;
    
    // 輔助方法
    void LogEvent(const std::string& action, std::type_index eventType, const std::string& details = "") const;
    std::string GetTypeName(std::type_index typeIndex) const;
};

// EventManager 的輔助類別 - 事件監聽器基類
class EventListener {
public:
    explicit EventListener(IEventManager* eventManager);
    virtual ~EventListener();
    
    // 設置事件管理器 (用於延遲初始化)
    void SetEventManager(IEventManager* eventManager) { eventManager_ = eventManager; }
    
protected:
    // 便利方法來註冊事件處理器
    template<typename EventType>
    void ListenTo(EventHandler<EventType> handler);
    
    template<typename EventType>
    void StopListening();
    
    // 發送事件的便利方法
    template<typename EventType>
    void Emit(const EventType& event);
    
    template<typename EventType>
    void EmitQueued(const EventType& event);

private:
    IEventManager* eventManager_;
    std::vector<std::type_index> subscribedTypes_;
};

// EventListener 模板實作
template<typename EventType>
void EventListener::ListenTo(EventHandler<EventType> handler) {
    if (eventManager_) {
        eventManager_->Subscribe<EventType>(handler);
        subscribedTypes_.push_back(std::type_index(typeid(EventType)));
    }
}

template<typename EventType>
void EventListener::StopListening() {
    if (eventManager_) {
        eventManager_->Unsubscribe<EventType>();
        
        // 從訂閱列表中移除
        auto it = std::find(subscribedTypes_.begin(), subscribedTypes_.end(), 
                           std::type_index(typeid(EventType)));
        if (it != subscribedTypes_.end()) {
            subscribedTypes_.erase(it);
        }
    }
}

template<typename EventType>
void EventListener::Emit(const EventType& event) {
    if (eventManager_) {
        eventManager_->Publish(event);
    }
}

template<typename EventType>
void EventListener::EmitQueued(const EventType& event) {
    if (eventManager_) {
        eventManager_->QueueEvent(event);
    }
}

// 便利巨集 - 簡化事件處理器註冊
#define LISTEN_TO_EVENT(EventType, handler) \
    ListenTo<EventType>([this](const EventType& event) { handler(event); })

#define STOP_LISTENING_TO_EVENT(EventType) \
    StopListening<EventType>()

// 便利巨集 - 簡化事件發送
#define EMIT_EVENT(EventType, ...) \
    Emit(EventType{__VA_ARGS__})

#define EMIT_QUEUED_EVENT(EventType, ...) \
    EmitQueued(EventType{__VA_ARGS__})