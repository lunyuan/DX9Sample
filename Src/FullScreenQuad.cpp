#include "FullScreenQuad.h"

// Factory 函式實作
std::unique_ptr<IFullScreenQuad> CreateFullScreenQuad() {
  return std::make_unique<FullScreenQuad>();
}

STDMETHODIMP FullScreenQuad::Init(IDirect3DDevice9* dev,
  const std::wstring& fxFile) {
  if (!dev || fxFile.empty()) return E_INVALIDARG;
  HRESULT hr = D3DXCreateEffectFromFile(dev, fxFile.c_str(),
    nullptr, nullptr, 0, nullptr, &fx_, nullptr);
  if (FAILED(hr)) return hr;
  hr = dev->CreateVertexBuffer(sizeof(VS_POST_VERTEX) * 4,
    D3DUSAGE_WRITEONLY, FVF_POST, D3DPOOL_MANAGED, &vb_, nullptr);
  if (FAILED(hr)) return hr;
  VS_POST_VERTEX* v;
  vb_->Lock(0, 0, reinterpret_cast<void**>(&v), 0);
  v[0] = { -0.5f,-0.5f,0,1,0,0 }; v[1] = { 799.5f,-0.5f,0,1,1,0 };
  v[2] = { 799.5f,599.5f,0,1,1,1 }; v[3] = { -0.5f,599.5f,0,1,0,1 };
  vb_->Unlock();
  return S_OK;
}

STDMETHODIMP FullScreenQuad::Render(IDirect3DDevice9* dev,
  IDirect3DTexture9* input) {
  if (!dev || !input) return E_INVALIDARG;
  fx_->SetTechnique(fx_->GetTechniqueByName("Tech_PostProcess"));
  UINT passes;
  fx_->Begin(&passes, 0);
  for (UINT i = 0; i < passes; ++i) {
    fx_->BeginPass(i);
    fx_->SetTexture("g_InputTexture", input);
    dev->SetFVF(FVF_POST);
    dev->SetStreamSource(0, vb_.Get(), 0, sizeof(VS_POST_VERTEX));
    dev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
    fx_->EndPass();
  }
  fx_->End();
  return S_OK;
}