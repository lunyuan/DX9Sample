#pragma once

#include "IRenderTargetManager.h"
#include <stdexcept>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RenderTargetManager : public IRenderTargetManager {
public:
  // Step 0: 建構函式，不拋例外
  RenderTargetManager(
    ComPtr<IDirect3DDevice9> device,
    const std::vector<RenderTargetDesc>& descs
  ) noexcept;

  ~RenderTargetManager() override = default;

  // Step 1: 初始化或重設 Render Target
  HRESULT Initialize(
    ComPtr<IDirect3DDevice9> device,
    const std::vector<RenderTargetDesc>& descs
  ) override;

  // Step 2: 裝置遺失時釋放所有 Default Pool 資源
  void OnDeviceLost() noexcept override;

  // Step 3: 裝置重設時重建所有 Render Target
  HRESULT OnDeviceReset() override;

  // Step 4: 取得指定索引的 Surface
  ComPtr<IDirect3DSurface9> GetSurface(size_t index) const override;

  // Step 5: 清除所有資源（保留 descs_ 以便重設）
  void Cleanup() noexcept override;

private:
  ComPtr<IDirect3DDevice9> device_;
  std::vector<RenderTargetDesc> descs_;
  std::vector<ComPtr<IDirect3DTexture9>> textures_;
  std::vector<ComPtr<IDirect3DSurface9>> surfaces_;
};

// Factory 實作
inline std::unique_ptr<IRenderTargetManager> CreateRenderTargetManager(
  ComPtr<IDirect3DDevice9> device,
  const std::vector<RenderTargetDesc>& descs
) {
  auto mgr = std::make_unique<RenderTargetManager>(device, descs);
  mgr->Initialize(device, descs);
  return mgr;
}
