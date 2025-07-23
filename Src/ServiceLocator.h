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
    
    // 設置現代架構服務
    void SetAssetManager(IAssetManager* assetManager) { assetManager_ = assetManager; }
    void SetUIManager(IUIManager* uiManager) { uiManager_ = uiManager; }
    void SetEventManager(IEventManager* eventManager) { eventManager_ = eventManager; }
    void SetConfigManager(IConfigManager* configManager) { configManager_ = configManager; }
    void SetSceneManager(ISceneManager* sceneManager) { sceneManager_ = sceneManager; }
    void SetDevice(IDirect3DDevice9* device) { device_ = device; }
    void SetCameraController(ICameraController* cameraController) { cameraController_ = cameraController; }
    
    // 設置舊架構服務
    void SetTextureManager(ITextureManager* textureManager) { textureManager_ = textureManager; }
    void SetEffectManager(IEffectManager* effectManager) { effectManager_ = effectManager; }
    void SetD3DContext(ID3DContext* d3dContext) { d3dContext_ = d3dContext; }
    void SetModelManager(IModelManager* modelManager) { modelManager_ = modelManager; }
    void SetLightManager(ILightManager* lightManager) { lightManager_ = lightManager; }
    void SetScene3D(IScene3D* scene3D) { scene3D_ = scene3D; }
    void SetInputHandler(IInputHandler* inputHandler) { inputHandler_ = inputHandler; }
    void SetPostProcessor(IFullScreenQuad* postProcessor) { postProcessor_ = postProcessor; }
    
    // IServiceLocator 介面實作 - 現代架構
    IAssetManager* GetAssetManager() const override { return assetManager_; }
    IUIManager* GetUIManager() const override { return uiManager_; }
    IEventManager* GetEventManager() const override { return eventManager_; }
    IConfigManager* GetConfigManager() const override { return configManager_; }
    ISceneManager* GetSceneManager() const override { return sceneManager_; }
    IDirect3DDevice9* GetDevice() const override { return device_; }
    ICameraController* GetCameraController() const override { return cameraController_; }
    
    // IServiceLocator 介面實作 - 舊架構
    ITextureManager* GetTextureManager() const override { return textureManager_; }
    IEffectManager* GetEffectManager() const override { return effectManager_; }
    ID3DContext* GetD3DContext() const override { return d3dContext_; }
    IModelManager* GetModelManager() const override { return modelManager_; }
    ILightManager* GetLightManager() const override { return lightManager_; }
    IScene3D* GetScene3D() const override { return scene3D_; }
    IInputHandler* GetInputHandler() const override { return inputHandler_; }
    IFullScreenQuad* GetPostProcessor() const override { return postProcessor_; }
    
    bool IsValid() const override { return device_ != nullptr; }
    
    // 驗證所有必要服務是否已設置
    bool ValidateServices() const;
    
    // 除錯
    void PrintServiceStatus() const;

private:
    // 現代架構服務
    IAssetManager* assetManager_;
    IUIManager* uiManager_;
    IEventManager* eventManager_;
    IConfigManager* configManager_;
    ISceneManager* sceneManager_;
    IDirect3DDevice9* device_;
    ICameraController* cameraController_;
    
    // 舊架構服務
    ITextureManager* textureManager_;
    IEffectManager* effectManager_;
    ID3DContext* d3dContext_;
    IModelManager* modelManager_;
    ILightManager* lightManager_;
    IScene3D* scene3D_;
    IInputHandler* inputHandler_;
    IFullScreenQuad* postProcessor_;
};