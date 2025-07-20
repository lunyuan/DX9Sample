#pragma once
#include <d3d9.h>
#include <DirectXMath.h>
#include "ILightManager.h"
#include "IUIManager.h"
#include <string>
#include <memory>

using namespace DirectX;

/// <summary>
/// 3D 模型渲染介面
/// </summary>
struct IScene3D {
  virtual ~IScene3D() = default;
  virtual HRESULT Init(IDirect3DDevice9* dev,
    ILightManager* lightMgr,
    const std::wstring& meshFile,
    const std::wstring& texFile) = 0;
  virtual HRESULT Render(IDirect3DDevice9* dev,
    const XMMATRIX& view,
    const XMMATRIX& proj, IUIManager* uiManager = nullptr) = 0;
};

/// <summary>Factory 函式：建立預設實作的 Scene3D。</summary>
std::unique_ptr<IScene3D> CreateScene3D();