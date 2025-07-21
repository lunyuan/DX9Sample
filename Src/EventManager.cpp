#include "EventManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>

// Factory 函式實作
std::unique_ptr<IEventManager> CreateEventManager() {
    return std::make_unique<EventManager>();
}

EventManager::EventManager()
    : processedEventCount_(0)
    , publishedEventCount_(0)
    , debugMode_(false)
{
    if (debugMode_) {
    }
}

EventManager::~EventManager() {
    Clear();
    
    if (debugMode_) {
    }
}

void EventManager::SubscribeInternal(std::type_index eventType, GenericEventHandler handler) {
    std::unique_lock<std::shared_mutex> lock(handlerMutex_);
    
    eventHandlers_[eventType].push_back(handler);
    
    LogEvent("Subscribe", eventType, "Handler count: " + std::to_string(eventHandlers_[eventType].size()));
}

void EventManager::UnsubscribeInternal(std::type_index eventType) {
    std::unique_lock<std::shared_mutex> lock(handlerMutex_);
    
    auto it = eventHandlers_.find(eventType);
    if (it != eventHandlers_.end()) {
        size_t handlerCount = it->second.size();
        eventHandlers_.erase(it);
        
        LogEvent("Unsubscribe", eventType, "Removed " + std::to_string(handlerCount) + " handlers");
    }
}

void EventManager::PublishInternal(std::type_index eventType, const IEvent& event) {
    std::shared_lock<std::shared_mutex> lock(handlerMutex_);
    
    auto it = eventHandlers_.find(eventType);
    if (it != eventHandlers_.end()) {
        LogEvent("Publish", eventType, "Handler count: " + std::to_string(it->second.size()));
        
        // 調用所有註冊的處理器
        for (const auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                std::cerr << "EventManager: Exception in event handler for " 
                          << GetTypeName(eventType) << ": " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "EventManager: Unknown exception in event handler for " 
                          << GetTypeName(eventType) << std::endl;
            }
        }
        
        publishedEventCount_++;
    } else {
        LogEvent("Publish", eventType, "No handlers registered");
    }
}

void EventManager::QueueEventInternal(std::type_index eventType, std::unique_ptr<IEvent> event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    eventQueue_.emplace(eventType, std::move(event));
    
    LogEvent("Queue", eventType, "Queue size: " + std::to_string(eventQueue_.size()));
}

void EventManager::ProcessEvents() {
    // 將事件複製到臨時佇列，避免在處理時持有鎖
    std::queue<QueuedEvent> tempQueue;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::swap(tempQueue, eventQueue_);
    }
    
    if (debugMode_ && !tempQueue.empty()) {
    }
    
    size_t processed = 0;
    while (!tempQueue.empty()) {
        auto& queuedEvent = tempQueue.front();
        
        // 發布事件（現在不持有 queueMutex_）
        PublishInternal(queuedEvent.type, *queuedEvent.event);
        
        tempQueue.pop();
        processed++;
    }
    
    processedEventCount_ += processed;
    
    if (debugMode_ && processed > 0) {
    }
}

void EventManager::Clear() {
    {
        std::unique_lock<std::shared_mutex> handlerLock(handlerMutex_);
        eventHandlers_.clear();
    }
    
    {
        std::lock_guard<std::mutex> queueLock(queueMutex_);
        while (!eventQueue_.empty()) {
            eventQueue_.pop();
        }
    }
    
    if (debugMode_) {
    }
}

size_t EventManager::GetHandlerCount() const {
    std::shared_lock<std::shared_mutex> lock(handlerMutex_);
    
    size_t total = 0;
    for (const auto& pair : eventHandlers_) {
        total += pair.second.size();
    }
    return total;
}

size_t EventManager::GetQueuedEventCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return eventQueue_.size();
}

void EventManager::PrintEventInfo() const {
    std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
    std::lock_guard<std::mutex> queueLock(queueMutex_);
    
    // EventManager debug info removed for minimal logging
}

void EventManager::ResetStatistics() {
    processedEventCount_ = 0;
    publishedEventCount_ = 0;
    
    if (debugMode_) {
    }
}

void EventManager::LogEvent(const std::string& action, std::type_index eventType, const std::string& details) const {
    if (debugMode_) {
    }
}

std::string EventManager::GetTypeName(std::type_index typeIndex) const {
    const char* name = typeIndex.name();
    
    // 簡化類型名稱顯示
    std::string typeName = name;
    
    // 移除 "struct" 前綴
    if (typeName.find("struct ") == 0) {
        typeName = typeName.substr(7);
    }
    
    // 移除 "class" 前綴
    if (typeName.find("class ") == 0) {
        typeName = typeName.substr(6);
    }
    
    // 移除命名空間前綴 "Events::"
    size_t pos = typeName.find("Events::");
    if (pos != std::string::npos) {
        typeName = typeName.substr(pos + 8);
    }
    
    return typeName;
}

// EventListener 實作
EventListener::EventListener(IEventManager* eventManager)
    : eventManager_(eventManager)
{
}

EventListener::~EventListener() {
    // 只在 eventManager_ 不為 nullptr 時清理訂閱
    if (eventManager_) {
        try {
            for (const auto& type : subscribedTypes_) {
                static_cast<EventManager*>(eventManager_)->UnsubscribeInternal(type);
            }
        } catch (...) {
            // 忽略所有異常，避免在析構函數中拋出異常
        }
    }
    
    // 清理本地資料
    subscribedTypes_.clear();
    eventManager_ = nullptr;
}