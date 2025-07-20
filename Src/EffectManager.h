#pragma once
#include "IEffectManager.h"

class EffectManager : public IEffectManager {
public:
  // Load or Cache
  STDMETHOD(LoadEffect)(IDirect3DDevice9* device,
    const std::wstring& file,
    ID3DXEffect** outFx) override;
private:
  std::unordered_map<std::wstring, ComPtr<ID3DXEffect>> cache_;
};