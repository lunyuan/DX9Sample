#pragma once

#include <filesystem>
#include <string_view>
#include <memory>
#include <wrl/client.h>
#include <d3d9.h>

using Microsoft::WRL::ComPtr;

/// <summary>
/// 介面：管理貼圖的載入、快取與釋放。
/// </summary>
struct ITextureManager {
  virtual ~ITextureManager() = default;

  /// <summary>初始化或重設 D3D Device，並清除舊快取以便重新載入貼圖。</summary>
  virtual void Initialize(ComPtr<IDirect3DDevice9> device) = 0;

  /// <summary>載入貼圖並快取；若已存在快取則直接回傳。</summary>
  virtual std::shared_ptr<IDirect3DBaseTexture9> Load(const std::filesystem::path& filepath) = 0;

  /// <summary>取得已快取的貼圖，若不存在則回傳 nullptr。</summary>
  virtual std::shared_ptr<IDirect3DBaseTexture9> Get(std::string_view key) const = 0;

  /// <summary>清除所有快取貼圖。</summary>
  virtual void Clear() noexcept = 0;
};

/// <summary>Factory 函式：建立預設實作的 TextureManager。</summary>
std::unique_ptr<ITextureManager> CreateTextureManager(ComPtr<IDirect3DDevice9> device);

