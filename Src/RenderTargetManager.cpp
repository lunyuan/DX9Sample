#include "RenderTargetManager.h"

RenderTargetManager::RenderTargetManager(
  ComPtr<IDirect3DDevice9> device,
  const std::vector<RenderTargetDesc>& descs
) noexcept
  : device_(device),
  descs_(descs) {
}

HRESULT RenderTargetManager::Initialize(
  ComPtr<IDirect3DDevice9> device,
  const std::vector<RenderTargetDesc>& descs
) {
  // Step 1: 參數檢查
  if (!device.Get()) {
    return E_POINTER;
  }
  if (descs.empty()) {
    return E_INVALIDARG;
  }

  device_ = device;
  descs_ = descs;
  textures_.clear();
  surfaces_.clear();

  // Step 1.1: 預留空間以避免多次配置
  textures_.reserve(descs_.size());
  surfaces_.reserve(descs_.size());

  // Step 1.2: 建立每個 Render Target
  for (size_t i = 0; i < descs_.size(); ++i) {
    const auto& d = descs_[i];
    ComPtr<IDirect3DTexture9> tex;

    HRESULT hr = device_->CreateTexture(
      d.width, d.height, 1,
      D3DUSAGE_RENDERTARGET,
      d.format,
      D3DPOOL_DEFAULT,
      tex.GetAddressOf(),
      nullptr
    );
    if (FAILED(hr)) {
      return hr;  // Step 1.2 Error Check
    }

    ComPtr<IDirect3DSurface9> surf;
    hr = tex->GetSurfaceLevel(0, surf.GetAddressOf());
    if (FAILED(hr)) {
      return hr;  // Step 1.2 Error Check
    }

    textures_.push_back(tex);
    surfaces_.push_back(surf);
  }

  return S_OK;
}

void RenderTargetManager::OnDeviceLost() noexcept {
  // Step 2: 釋放所有 Default Pool 資源
  textures_.clear();
  surfaces_.clear();
}

HRESULT RenderTargetManager::OnDeviceReset() {
  // Step 3: 檢查裝置再重建
  if (!device_.Get()) {
    return E_FAIL;
  }
  return Initialize(device_, descs_);  // Step 3 Error Check
}

ComPtr<IDirect3DSurface9> RenderTargetManager::GetSurface(size_t index) const {
  // Step 4: 參數檢查
  if (index >= surfaces_.size()) {
    throw std::out_of_range("RenderTargetManager: index out of range");
  }
  return surfaces_[index];
}

void RenderTargetManager::Cleanup() noexcept {
  // Step 5: 僅清除 D3D 資源，保留 descs_ 以便後續 Reset
  surfaces_.clear();
  textures_.clear();
  // descs_ 與 device_ 不清除
}
