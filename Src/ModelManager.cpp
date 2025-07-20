// Step 0: ModelManager.cpp 必要 include // error check
#include "ModelManager.h"
#include <stdexcept>            // std::invalid_argument

// Factory 函式實作
std::unique_ptr<IModelManager> CreateModelManager(std::unique_ptr<IModelLoader> loader, ITextureManager* textureManager) {
  return std::make_unique<ModelManager>(std::move(loader), textureManager);
}

// Step 1: Constructor 注入 // error check
ModelManager::ModelManager(std::unique_ptr<IModelLoader> loader, ITextureManager* textureManager)
  : loader_(std::move(loader)), textureManager_(textureManager) {
  if (!loader_) {
    throw std::invalid_argument("ModelManager requires a valid IModelLoader");
  }
  if (!textureManager_) {
    throw std::invalid_argument("ModelManager requires a valid ITextureManager");
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

// Step 5: 載入特定模型實作
bool ModelManager::LoadModel(
  const std::filesystem::path& file,
  const std::string& modelName,
  IDirect3DDevice9* device
) {
  // 引數檢查
  if (file.empty()) {
    throw std::invalid_argument("ModelManager::LoadModel: empty file path");
  }
  if (modelName.empty()) {
    throw std::invalid_argument("ModelManager::LoadModel: empty model name");
  }
  if (!device) {
    throw std::invalid_argument("ModelManager::LoadModel: device is null");
  }

  // 載入檔案中的所有模型
  auto allModels = loader_->Load(file, device);
  
  // 檢查指定的模型是否存在
  auto it = allModels.find(modelName);
  if (it == allModels.end()) {
    return false; // 模型不存在
  }
  
  // 添加到已載入的模型中
  models_[modelName] = std::move(it->second);
  return true;
}

// Step 6: 載入特定模型並重命名實作
bool ModelManager::LoadModelAs(
  const std::filesystem::path& file,
  const std::string& modelName,
  const std::string& aliasName,
  IDirect3DDevice9* device
) {
  // 引數檢查
  if (file.empty()) {
    throw std::invalid_argument("ModelManager::LoadModelAs: empty file path");
  }
  if (modelName.empty()) {
    throw std::invalid_argument("ModelManager::LoadModelAs: empty model name");
  }
  if (aliasName.empty()) {
    throw std::invalid_argument("ModelManager::LoadModelAs: empty alias name");
  }
  if (!device) {
    throw std::invalid_argument("ModelManager::LoadModelAs: device is null");
  }

  // 載入檔案中的所有模型
  auto allModels = loader_->Load(file, device);
  
  // 檢查指定的模型是否存在
  auto it = allModels.find(modelName);
  if (it == allModels.end()) {
    return false; // 模型不存在
  }
  
  // 使用別名添加到已載入的模型中
  models_[aliasName] = std::move(it->second);
  return true;
}

// Step 7: 獲取已載入模型列表實作
std::vector<std::string> ModelManager::GetLoadedModelNames() const {
  std::vector<std::string> names;
  names.reserve(models_.size());
  
  for (const auto& pair : models_) {
    names.push_back(pair.first);
  }
  
  return names;
}

// Step 7a: 查詢檔案中包含的模型列表實作
std::vector<std::string> ModelManager::GetAvailableModels(const std::filesystem::path& file) const {
  if (!loader_) {
    return {};
  }
  
  return loader_->GetModelNames(file);
}

// Step 8: 檢查模型是否存在實作
bool ModelManager::HasModel(const std::string& name) const noexcept {
  return models_.find(name) != models_.end();
}

// Step 9: 移除特定模型實作
bool ModelManager::RemoveModel(const std::string& name) {
  auto it = models_.find(name);
  if (it != models_.end()) {
    models_.erase(it);
    return true;
  }
  return false;
}

// Step 10: Clear 實作 // error check
void ModelManager::Clear() noexcept {
  models_.clear();
}
