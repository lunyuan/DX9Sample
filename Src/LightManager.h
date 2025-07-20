#pragma once
#include "ILightManager.h"

class LightManager : public ILightManager {
public:
  void AddLight(ILight* light) override {
    lights_.push_back(light);
  }

  void ApplyAll(IDirect3DDevice9* dev) const override {
    for (DWORD i = 0; i < lights_.size(); ++i) {
      lights_[i]->Apply(dev, i);
    }
    // 環境光
    dev->SetRenderState(D3DRS_AMBIENT, 0x00404040);
  }

private:
  std::vector<ILight*> lights_;
};

// Factory 函式實作
inline std::unique_ptr<ILightManager> CreateLightManager() {
  return std::make_unique<LightManager>();
}