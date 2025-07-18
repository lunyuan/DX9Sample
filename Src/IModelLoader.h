#pragma once
#include <filesystem>
#include <map>
#include "ModelData.h"  

class IModelLoader {
public:
  virtual ~IModelLoader() = default;
  [[nodiscard]] virtual std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const = 0;
};