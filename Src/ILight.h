#pragma once
#include <d3d9.h>
#include <d3dx9math.h>

/// <summary>抽象光源介面</summary>
struct ILight {
  virtual ~ILight() = default;
  /// 把自己的 D3DLIGHT9 設定到 device 上 (index 表示第幾號光源)
  virtual void Apply(IDirect3DDevice9* dev, DWORD index) const = 0;
};
