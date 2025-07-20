#pragma once
#include <filesystem>
#include <map>
#include <string>
#include "IModelLoader.h"
#include "ModelData.h"

class XModelLoader : public IModelLoader {
public:
  [[nodiscard]] std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const override;
  
  [[nodiscard]] std::vector<std::string>
    GetModelNames(const std::filesystem::path& file) const override;
};
