#include "D3DContext.h"
#include <stdexcept>

// Factory 函式實作
std::unique_ptr<ID3DContext> CreateD3DContext() {
  return std::make_unique<D3DContext>();
}

STDMETHODIMP D3DContext::Init(HWND hwnd,
  UINT width, UINT height,
  D3DDEVTYPE devType,
  DWORD behaviorFlags) {
  if (!hwnd || width == 0 || height == 0) return E_INVALIDARG;
  
  // 建立 Direct3D
  d3d_ = Direct3DCreate9(D3D_SDK_VERSION);
  if (!d3d_) return E_FAIL;

  // 取得顯示模式格式
  D3DDISPLAYMODE d3ddm;
  HRESULT hr = d3d_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
  if (FAILED(hr)) return hr;

  // 設定呈現參數
  ZeroMemory(&pp_, sizeof(pp_));
  pp_.BackBufferWidth = width;
  pp_.BackBufferHeight = height;
  pp_.BackBufferFormat = d3ddm.Format;  // 使用當前顯示格式
  pp_.BackBufferCount = 1;
  pp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp_.hDeviceWindow = hwnd;
  pp_.Windowed = TRUE;
  pp_.EnableAutoDepthStencil = TRUE;
  
  // 嘗試不同的深度緩衝格式
  if (SUCCEEDED(d3d_->CheckDeviceFormat(D3DADAPTER_DEFAULT, devType, d3ddm.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8))) {
    pp_.AutoDepthStencilFormat = D3DFMT_D24S8;
  } else if (SUCCEEDED(d3d_->CheckDeviceFormat(D3DADAPTER_DEFAULT, devType, d3ddm.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16))) {
    pp_.AutoDepthStencilFormat = D3DFMT_D16;
  } else {
    pp_.AutoDepthStencilFormat = D3DFMT_D24X8;
  }
  
  pp_.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  // 建立裝置
  hr = d3d_->CreateDevice(
    D3DADAPTER_DEFAULT, devType, hwnd,
    behaviorFlags, &pp_, &device_);
  return hr;
}

STDMETHODIMP D3DContext::GetDevice(IDirect3DDevice9** outDevice) {
  if (!device_ || !outDevice) return E_POINTER;
  *outDevice = device_.Get();
  (*outDevice)->AddRef();
  return S_OK;
}

STDMETHODIMP D3DContext::Reset() {
  if (!device_) return E_FAIL;
  HRESULT hr = device_->TestCooperativeLevel();
  if (hr == D3DERR_DEVICENOTRESET) {
    hr = device_->Reset(&pp_);
  }
  return hr;
}

STDMETHODIMP D3DContext::BeginScene() {
  if (!device_) return E_FAIL;
  return device_->BeginScene();
}

STDMETHODIMP D3DContext::EndScene() {
  if (!device_) return E_FAIL;
  return device_->EndScene();
}

STDMETHODIMP D3DContext::Present() {
  if (!device_) return E_FAIL;
  return device_->Present(nullptr, nullptr, nullptr, nullptr);
}

STDMETHODIMP D3DContext::Clear(DWORD clearFlags, D3DCOLOR color, float z, DWORD stencil) {
  if (!device_) return E_FAIL;
  return device_->Clear(0, nullptr, clearFlags, color, z, stencil);
}
