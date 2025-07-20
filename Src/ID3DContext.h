#pragma once

#include <d3d9.h>
#include <memory>

/// <summary>
/// 管理 Direct3D9 設備初始化與 Lost/Reset
/// </summary>
struct ID3DContext {
  virtual ~ID3DContext() = default;
  STDMETHOD(Init)(HWND hwnd,
    UINT width, UINT height,
    D3DDEVTYPE devType = D3DDEVTYPE_HAL,
    DWORD behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING) = 0;
  STDMETHOD(GetDevice)(IDirect3DDevice9** outDevice) = 0;
  STDMETHOD(Reset)() = 0;
  STDMETHOD(BeginScene)() = 0;
  STDMETHOD(EndScene)() = 0;
  STDMETHOD(Present)() = 0;
  STDMETHOD(Clear)(DWORD clearFlags, D3DCOLOR color, float z, DWORD stencil) = 0;
};

/// <summary>Factory 函式：建立預設實作的 D3DContext。</summary>
std::unique_ptr<ID3DContext> CreateD3DContext();