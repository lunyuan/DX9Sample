#pragma once

#include <memory>               // std::unique_ptr
#include <filesystem>           // std::filesystem::path
#include <map>                  // std::map
#include <string>               // std::string
#include "IModelManager.h"
#include "IModelLoader.h"       // IModelLoader 定義
#include "ModelData.h"          // ModelData 定義
#include "ITextureManager.h"    // ITextureManager 定義

class ModelManager : public IModelManager {

public:
  explicit ModelManager(std::unique_ptr<IModelLoader> loader, ITextureManager* textureManager);
  void Initialize(std::unique_ptr<IModelLoader> loader) override;

  // 載入所有模型
  void LoadModels(const std::filesystem::path& file, IDirect3DDevice9* device) override;
  
  // 載入特定模型
  bool LoadModel(const std::filesystem::path& file, const std::string& modelName, IDirect3DDevice9* device) override;
  
  // 載入特定模型並重命名
  bool LoadModelAs(const std::filesystem::path& file, const std::string& modelName, const std::string& aliasName, IDirect3DDevice9* device) override;
  
  // 獲取已載入模型列表
  std::vector<std::string> GetLoadedModelNames() const override;
  
  // 查詢檔案中包含的模型列表
  std::vector<std::string> GetAvailableModels(const std::filesystem::path& file) const override;
  
  // 檢查模型是否存在
  bool HasModel(const std::string& name) const noexcept override;
  
  [[nodiscard]] const ModelData* GetModel(const std::string& name) const noexcept override;
  void Clear() noexcept override;
  
  // 移除特定模型
  bool RemoveModel(const std::string& name) override;

private:
  // 使用 std::unique_ptr 管理 IModelLoader 的實例
  std::unique_ptr<IModelLoader> loader_;
  // TextureManager 用於模型紋理管理
  ITextureManager* textureManager_;
  // 模型名稱到 ModelData 的映射
  std::map<std::string, ModelData> models_;
};