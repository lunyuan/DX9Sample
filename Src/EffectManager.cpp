#include "EffectManager.h"
#include <d3dx9.h>

// Factory 函式實作
std::unique_ptr<IEffectManager> CreateEffectManager() {
  return std::make_unique<EffectManager>();
}

STDMETHODIMP EffectManager::LoadEffect(IDirect3DDevice9* device,
  const std::wstring& file,
  ID3DXEffect** outFx) {
  if (!device || !outFx) return E_POINTER;
  auto it = cache_.find(file);
  if (it != cache_.end()) {
    *outFx = it->second.Get();
    (*outFx)->AddRef();
    return S_OK;
  }
  ComPtr<ID3DXEffect> fx;
  HRESULT hr = D3DXCreateEffectFromFile(device,
    file.c_str(),
    nullptr, nullptr, 0, nullptr, &fx, nullptr);
  if (FAILED(hr)) return hr;
  cache_[file] = fx;
  *outFx = fx.Get();
  (*outFx)->AddRef();
  return S_OK;
}