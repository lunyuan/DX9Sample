#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>
#include <memory>

// Forward declarations
struct ITextureManager;
struct IEffectManager;
struct ID3DContext;
struct IModelManager;
struct ILightManager;
struct IScene3D;
struct IUIManager;
struct IInputHandler;
struct ICameraController;
struct IFullScreenQuad;
struct ISceneManager;
struct IAssetManager;
struct IEventManager;
struct IConfigManager;

struct IEngineContext {
  virtual ~IEngineContext() = default;
  virtual HRESULT Initialize(
    HWND hwnd,
    UINT width,
    UINT height
  ) = 0;
  virtual HRESULT LoadAssets(
    const std::wstring& modelFile,
    const std::wstring& textureFile
  ) = 0;
  virtual HRESULT Run() = 0;
  // 取得子系統介面 (舊系統)
  virtual ITextureManager* GetTextureManager() = 0;
  virtual IEffectManager* GetEffectManager() = 0;
  virtual ID3DContext* GetD3DContext() = 0;
  virtual IModelManager* GetModelManager() = 0;
  virtual ILightManager* GetLightManager() = 0;
  virtual IScene3D* GetScene3D() = 0;
  virtual IUIManager* GetUIManager() = 0;
  virtual IInputHandler* GetInputHandler() = 0;
  virtual ICameraController* GetCameraController() = 0;
  virtual IFullScreenQuad* GetPostProcessor() = 0;
  
  // 取得新架構系統介面
  virtual ISceneManager* GetSceneManager() = 0;
  virtual IAssetManager* GetAssetManager() = 0;
  virtual IEventManager* GetEventManager() = 0;
  virtual IConfigManager* GetConfigManager() = 0;
};

// Factory 函式
std::unique_ptr<IEngineContext> CreateEngineContext();