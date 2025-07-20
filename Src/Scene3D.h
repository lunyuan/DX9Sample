#pragma once
#include "IScene3D.h"
#include <wrl/client.h>
#include <d3dx9.h>

using Microsoft::WRL::ComPtr;

/// <summary>
/// Scene3D 實作：載入、光照及渲染
/// </summary>
class Scene3D : public IScene3D {
public:
  HRESULT Init(IDirect3DDevice9* dev,
    ILightManager* lightMgr,
    const std::wstring& meshFile,
    const std::wstring& texFile) override;
  HRESULT STDMETHODCALLTYPE Render(IDirect3DDevice9* dev,
    const XMMATRIX& view,
    const XMMATRIX& proj, IUIManager* uiManager = nullptr) override;
private:
  ComPtr<ID3DXMesh>      mesh_;
  ComPtr<IDirect3DTexture9> tex_;
  ComPtr<ID3DXEffect>    fx_;

  ILightManager* lightMgr_ = nullptr;
  D3DXHANDLE             hView_ = nullptr;
  D3DXHANDLE             hProj_ = nullptr;
};