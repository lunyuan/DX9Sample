#pragma once

#include <vector>
#include <memory>
#include <wrl/client.h>
#include <d3d9.h>

using Microsoft::WRL::ComPtr;

/// <summary>
/// 描述 Render Target 大小、格式與多重取樣設定。
/// </summary>
struct RenderTargetDesc {
  UINT      width;       // 寬度
  UINT      height;      // 高度
  D3DFORMAT format;      // 紋理格式
  UINT      multiSample; // 多重取樣等級 (0 表示不使用多重取樣)
};

/// <summary>
/// 介面：管理多個 Render Target Surface 的建立、釋放與重建。
/// </summary>
struct IRenderTargetManager {
  virtual ~IRenderTargetManager() = default;

  /// <summary>
  /// 初始化或重設 D3D Device 及多個 Render Target。Device Reset 時呼。
  /// </summary>
  virtual HRESULT Initialize(ComPtr<IDirect3DDevice9> device,
    const std::vector<RenderTargetDesc>& descs) = 0;

  /// <summary>
  /// Device Lost 時釋放 Default Pool 資源。
  /// </summary>
  virtual void OnDeviceLost() noexcept = 0;

  /// <summary>
  /// Device Reset 時重建所有 Render Target。呼 OnDeviceReset。
  /// </summary>
  virtual HRESULT OnDeviceReset() = 0;

  /// <summary>
  /// 取得指定索引的 Render Target Surface。
  /// </summary>
  virtual ComPtr<IDirect3DSurface9> GetSurface(size_t index) const = 0;

  /// <summary>
  /// 清除所有 Render Target 資源。
  /// </summary>
  virtual void Cleanup() noexcept = 0;
};

/// <summary>Factory 函式：建立預設實作的 RenderTargetManager。</summary>
std::unique_ptr<IRenderTargetManager> CreateRenderTargetManager(
  ComPtr<IDirect3DDevice9> device,
  const std::vector<RenderTargetDesc>& descs);
