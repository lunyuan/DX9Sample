#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>
#include <memory>
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
  // 取得子系統介面
  virtual class ITextureManager* GetTextureManager() = 0;
  virtual class IEffectManager* GetEffectManager() = 0;
  virtual class ID3DContext* GetD3DContext() = 0;
  virtual class IModelManager* GetModelManager() = 0;
  virtual class ILightManager* GetLightManager() = 0;
  virtual class IScene3D* GetScene3D() = 0;
  virtual class IUIManager* GetUIManager() = 0;
  virtual class IInputHandler* GetInputHandler() = 0;
  virtual class ICameraController* GetCameraController() = 0;
  virtual class IFullScreenQuad* GetPostProcessor() = 0;
};

// Factory 函式
std::unique_ptr<IEngineContext> CreateEngineContext();