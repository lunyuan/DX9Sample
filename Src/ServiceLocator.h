#pragma once

#include "IScene.h"
#include <memory>

// Forward declarations
struct IAssetManager;
struct IUIManager;
struct IEventManager;
struct IConfigManager;
struct ISceneManager;
struct IDirect3DDevice9;
struct ICameraController;

class ServiceLocator : public IServiceLocator {
public:
    ServiceLocator();
    ~ServiceLocator() = default;
    
    // 設置服務
    void SetAssetManager(IAssetManager* assetManager) { assetManager_ = assetManager; }
    void SetUIManager(IUIManager* uiManager) { uiManager_ = uiManager; }
    void SetEventManager(IEventManager* eventManager) { eventManager_ = eventManager; }
    void SetConfigManager(IConfigManager* configManager) { configManager_ = configManager; }
    void SetSceneManager(ISceneManager* sceneManager) { sceneManager_ = sceneManager; }
    void SetDevice(IDirect3DDevice9* device) { device_ = device; }
    void SetCameraController(ICameraController* cameraController) { cameraController_ = cameraController; }
    
    // IServiceLocator 介面實作
    IAssetManager* GetAssetManager() const override { return assetManager_; }
    IUIManager* GetUIManager() const override { return uiManager_; }
    IEventManager* GetEventManager() const override { return eventManager_; }
    IConfigManager* GetConfigManager() const override { return configManager_; }
    ISceneManager* GetSceneManager() const override { return sceneManager_; }
    IDirect3DDevice9* GetDevice() const override { return device_; }
    ICameraController* GetCameraController() const override { return cameraController_; }
    bool IsValid() const override { return device_ != nullptr; }
    
    // 驗證所有必要服務是否已設置
    bool ValidateServices() const;
    
    // 除錯
    void PrintServiceStatus() const;

private:
    IAssetManager* assetManager_;
    IUIManager* uiManager_;
    IEventManager* eventManager_;
    IConfigManager* configManager_;
    ISceneManager* sceneManager_;
    IDirect3DDevice9* device_;
    ICameraController* cameraController_;
};