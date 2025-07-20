#pragma once

#include <wrl/client.h>
#include <d3dx9.h>
#include <string>
#include <unordered_map>
#include <memory>
using Microsoft::WRL::ComPtr;

/// <summary>
/// 管理 .fx Effect 快取，不負責 D3D 初始化
/// </summary>
struct IEffectManager {
  virtual ~IEffectManager() = default;
  STDMETHOD(LoadEffect)(IDirect3DDevice9* device,
    const std::wstring& file,
    ID3DXEffect** outFx) = 0;
};

/// <summary>Factory 函式：建立預設實作的 EffectManager。</summary>
std::unique_ptr<IEffectManager> CreateEffectManager();