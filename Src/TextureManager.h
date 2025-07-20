#pragma once

#include "ITextureManager.h"
#include <unordered_map>
#include <shared_mutex>
#include <format>
#include <stdexcept>
#include <d3dx9tex.h>

using Microsoft::WRL::ComPtr;

class TextureManager : public ITextureManager {
public:
  // 建構函式，不拋例外
  explicit TextureManager(ComPtr<IDirect3DDevice9> device) noexcept;

  ~TextureManager() override = default;

  TextureManager(const TextureManager&) = delete;
  TextureManager& operator=(const TextureManager&) = delete;
  TextureManager(TextureManager&&) noexcept = default;
  TextureManager& operator=(TextureManager&&) noexcept = default;

  // 初始化／重設 Device 並清除快取
  void Initialize(ComPtr<IDirect3DDevice9> device) override;

  // 載入貼圖並快取；若已存在快取直接回傳
  std::shared_ptr<IDirect3DBaseTexture9> Load(
    const std::filesystem::path& filepath
  ) override;

  // 取得已快取貼圖，若不存在回傳 nullptr
  std::shared_ptr<IDirect3DBaseTexture9> Get(
    std::string_view key
  ) const override;

  // 清除所有快取
  void Clear() noexcept override;

private:
  ComPtr<IDirect3DDevice9> device_;
  mutable std::shared_mutex    mutex_;
  std::unordered_map<std::string,
    std::shared_ptr<IDirect3DBaseTexture9>> cache_;
};

/// <summary>Factory 函式：建立預設實作的 TextureManager。</summary>
std::unique_ptr<ITextureManager> CreateTextureManager(ComPtr<IDirect3DDevice9> device);

