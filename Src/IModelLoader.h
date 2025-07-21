#pragma once
#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include "ModelData.h"  

struct IModelLoader {
  virtual ~IModelLoader() = default;
  
  // 載入檔案中的所有模型
  [[nodiscard]] virtual std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const = 0;
  
  // 獲取檔案中包含的模型名稱列表（不實際載入）
  [[nodiscard]] virtual std::vector<std::string>
    GetModelNames(const std::filesystem::path& file) const = 0;
};