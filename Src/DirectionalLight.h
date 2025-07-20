// === DirectionalLight.h ===
#pragma once
#include "ILight.h"

/// <summary>可自訂方向與顏色的方向光</summary>
class DirectionalLight : public ILight {
public:
  DirectionalLight(float r, float g, float b,
    float dx, float dy, float dz)
    : diffuse_{ r,g,b,1.0f },
    direction_{ dx, dy, dz } {
  }

  void Apply(IDirect3DDevice9* dev, DWORD index) const override {
    D3DLIGHT9 L{};
    L.Type = D3DLIGHT_DIRECTIONAL;
    L.Diffuse = diffuse_;
    L.Direction = direction_;
    dev->SetLight(index, &L);
    dev->LightEnable(index, TRUE);
  }

private:
  D3DXCOLOR    diffuse_;
  D3DXVECTOR3  direction_;
};