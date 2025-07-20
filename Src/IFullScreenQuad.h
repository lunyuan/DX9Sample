#pragma once
#include <d3d9.h>
#include <string>
#include <memory>

/// <summary>
/// 全螢幕後處理介面
/// </summary>
struct IFullScreenQuad {
  virtual ~IFullScreenQuad() = default;
  STDMETHOD(Init)(IDirect3DDevice9* device,
    const std::wstring& fxFile) = 0;
  STDMETHOD(Render)(IDirect3DDevice9* device,
    IDirect3DTexture9* input) = 0;
};

/// <summary>Factory 函式：建立預設實作的 FullScreenQuad。</summary>
std::unique_ptr<IFullScreenQuad> CreateFullScreenQuad();