#pragma once
#include "IModelLoader.h"
#include <filesystem>
#include <map>
#include <vector>
#include <string>

class GltfModelLoader : public IModelLoader {
public:
    // 載入檔案中的所有模型
    [[nodiscard]] std::map<std::string, ModelData>
        Load(const std::filesystem::path& file, IDirect3DDevice9* device) const override;
    
    // 獲取檔案中包含的模型名稱列表（不實際載入）
    [[nodiscard]] std::vector<std::string>
        GetModelNames(const std::filesystem::path& file) const override;
};