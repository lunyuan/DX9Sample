#pragma once
#include "IFullScreenQuad.h"
#include <d3dx9.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

struct VS_POST_VERTEX { float x, y, z, rhw, u, v; };
#define FVF_POST (D3DFVF_XYZRHW|D3DFVF_TEX1)

class FullScreenQuad : public IFullScreenQuad {
public:
  STDMETHOD(Init)(IDirect3DDevice9* device,
    const std::wstring& fxFile) override;
  STDMETHOD(Render)(IDirect3DDevice9* device,
    IDirect3DTexture9* input) override;
private:
  ComPtr<ID3DXEffect> fx_;
  ComPtr<IDirect3DVertexBuffer9> vb_;
};