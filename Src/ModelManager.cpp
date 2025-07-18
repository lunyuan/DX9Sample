// Step 0: ModelManager.cpp 必要 include // error check
#include "ModelManager.h"
#include <stdexcept>            // std::invalid_argument

// Step 1: Constructor 注入 // error check
ModelManager::ModelManager(std::unique_ptr<IModelLoader> loader)
  : loader_(std::move(loader)) {
  if (!loader_) {
    throw std::invalid_argument("ModelManager requires a valid IModelLoader");
  }
}

// Step 2: Initialize 可重置注入 // error check
void ModelManager::Initialize(std::unique_ptr<IModelLoader> loader) {
  loader_ = std::move(loader);
  models_.clear();
}

// Step 3: LoadModels 實作 // error check
void ModelManager::LoadModels(
  const std::filesystem::path& file,
  IDirect3DDevice9* device
) {
  // 引數檢查
  if (file.empty()) {
    throw std::invalid_argument("ModelManager::LoadModels: empty file path");
  }
  if (!device) {
    throw std::invalid_argument("ModelManager::LoadModels: device is null");
  }
  // 委派給注入的 loader
  models_ = loader_->Load(file, device);
}

// Step 4: GetModel 實作 // error check
const ModelData* ModelManager::GetModel(
  const std::string& name
) const noexcept {
  if (auto it = models_.find(name); it != models_.end()) {
    return &it->second;
  }
  return nullptr;
}

// Step 5: Clear 實作 // error check
void ModelManager::Clear() noexcept {
  models_.clear();
}
