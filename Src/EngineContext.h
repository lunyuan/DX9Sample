// EngineContext.h
#pragma once

#include <wrl/client.h>
#include <Windows.h>
#include "../Include/IEngineContext.h"
#include "../Include/ITextureManager.h"
#include "../Include/IEffectManager.h"
#include "../Include/ID3DContext.h"
#include "../Include/IModelManager.h"
#include "../Include/ILightManager.h"
#include "../Include/IScene3D.h"
#include "../Include/IUIManager.h"
#include "../Include/IInputHandler.h"
#include "../Include/ICameraController.h"
#include "../Include/IFullScreenQuad.h"
#include "../Include/IAssetManager.h"
#include "../Include/IConfigManager.h"
#include "../Include/ISceneManager.h"
// #include "IUISystem.h" // Removed - using UIManager only
#include "../Include/IEventManager.h"
#include "ServiceLocator.h"

using Microsoft::WRL::ComPtr;

class DirectionalLight;

class EngineContext : public IEngineContext {
public:
  EngineContext();
  ~EngineContext();

  // Step 1: 函式簽章確認
  STDMETHOD(Initialize)(
    HWND hwnd,
    UINT width,
    UINT height
    );

  STDMETHOD(LoadAssets)(
    const std::wstring& modelFile,
    const std::wstring& textureFile
    );

  STDMETHOD(Run)();

  // Step 2: 取得子系統介面 (舊系統)
  ITextureManager* GetTextureManager()   override;
  IEffectManager* GetEffectManager()    override;
  ID3DContext* GetD3DContext()       override;
  IModelManager* GetModelManager()     override;
  ILightManager* GetLightManager()     override;
  IScene3D* GetScene3D()          override;
  IUIManager* GetUIManager()        override;
  IInputHandler* GetInputHandler()     override;
  ICameraController* GetCameraController() override;
  IFullScreenQuad* GetPostProcessor()    override;
  
  // 取得新架構系統介面
  ISceneManager* GetSceneManager() override;
  IAssetManager* GetAssetManager() override;
  IEventManager* GetEventManager() override;
  IConfigManager* GetConfigManager() override;
  IServiceLocator* GetServices() override;

private:
  HWND                          hwnd_;
  UINT                          width_;
  UINT                          height_;

  // 核心系統
  std::unique_ptr<ITextureManager>      uiTextureManager_;
  std::unique_ptr<ITextureManager>      modelTextureManager_;
  std::unique_ptr<IEffectManager>       effectManager_;
  std::unique_ptr<ID3DContext>          d3dContext_;
  std::unique_ptr<IModelManager>        modelManager_;
  std::unique_ptr<ILightManager>        lightManager_;
  std::unique_ptr<IScene3D>             scene3D_;
  std::unique_ptr<IUIManager>           uiManager_;
  std::unique_ptr<IInputHandler>        inputHandler_;
  std::unique_ptr<ICameraController>    cameraController_;
  std::unique_ptr<IFullScreenQuad>      fullScreenQuad_;
  
  // 新架構系統 - 注意順序：eventManager_ 必須最後析構
  std::unique_ptr<IEventManager>        eventManager_;  // 最先創建，最後析構
  std::unique_ptr<IAssetManager>        assetManager_;
  std::unique_ptr<IConfigManager>       configManager_;
  std::unique_ptr<ISceneManager>        sceneManager_;
  std::unique_ptr<ServiceLocator>       serviceLocator_;
  
  DirectionalLight*                     dirLight_;  // 不擁有，由lightManager_管理
  
  // 輔助方法
  bool InitializeModernSystems();
  void CreateServiceLocator();
  bool LoadConfiguration();
};
