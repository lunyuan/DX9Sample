#pragma once
#include "ID3DContext.h"
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

/// <summary>
/// ID3DContext 實作：初始化 Device、處理 Lost/Reset
/// </summary>
class D3DContext : public ID3DContext {
public:
  // Step 1: Init Device // error check
  STDMETHOD(Init)(HWND hwnd,
    UINT width, UINT height,
    D3DDEVTYPE devType = D3DDEVTYPE_HAL,
    DWORD behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING) override;
  // Step 2: Get Device // error check
  STDMETHOD(GetDevice)(IDirect3DDevice9** outDevice) override;
  // Step 3: Reset upon Lost // error check
  STDMETHOD(Reset)() override;
  // Step 4: Rendering methods
  STDMETHOD(BeginScene)() override;
  STDMETHOD(EndScene)() override;
  STDMETHOD(Present)() override;
  STDMETHOD(Clear)(DWORD clearFlags, D3DCOLOR color, float z, DWORD stencil) override;
private:
  ComPtr<IDirect3D9>        d3d_;
  ComPtr<IDirect3DDevice9>  device_;
  D3DPRESENT_PARAMETERS     pp_ = {};
};