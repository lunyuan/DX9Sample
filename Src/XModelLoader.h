#pragma once
#include <filesystem>
#include <map>
#include <string>
#include "ModelData.h"

class XModelLoader {
public:
  [[nodiscard]] std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const;
};
