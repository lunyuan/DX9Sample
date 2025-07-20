#pragma once
#include "ILight.h"
#include <vector>
#include <memory>

struct ILightManager {
  virtual ~ILightManager() = default;
  virtual void AddLight(ILight* light) = 0;
  virtual void ApplyAll(IDirect3DDevice9* dev) const = 0;
};

/// <summary>Factory 函式：建立預設實作的 LightManager。</summary>
std::unique_ptr<ILightManager> CreateLightManager();

