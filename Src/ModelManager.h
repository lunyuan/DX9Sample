#pragma once

#include <memory>               // std::unique_ptr
#include <filesystem>           // std::filesystem::path
#include <map>                  // std::map
#include <string>               // std::string

#include "IModelLoader.h"       // IModelLoader 定義
#include "ModelData.h"          // ModelData 定義

class ModelManager {
public:
  explicit ModelManager(std::unique_ptr<IModelLoader> loader);
  void Initialize(std::unique_ptr<IModelLoader> loader);

  void LoadModels(const std::filesystem::path& file, IDirect3DDevice9* device);
  [[nodiscard]] const ModelData* GetModel(const std::string& name) const noexcept;
  void Clear() noexcept;

private:
  std::unique_ptr<IModelLoader>        loader_;
  std::map<std::string, ModelData>     models_;
};