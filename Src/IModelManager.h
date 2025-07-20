#pragma once

#include <memory>               // std::unique_ptr
#include <filesystem>           // std::filesystem::path
#include <map>                  // std::map
#include <string>               // std::string

// Forward declarations
struct IModelLoader;
struct ITextureManager;
struct ModelData;
struct IDirect3DDevice9;

struct IModelManager {
public:
  virtual ~IModelManager() = default;
  
  virtual void Initialize(std::unique_ptr<IModelLoader> loader) = 0;
  
  // 載入檔案中的所有模型
  virtual void LoadModels(const std::filesystem::path& file, IDirect3DDevice9* device) = 0;
  
  // 載入檔案中的特定模型（指定名稱）
  virtual bool LoadModel(const std::filesystem::path& file, const std::string& modelName, IDirect3DDevice9* device) = 0;
  
  // 載入檔案中的特定模型並指定新名稱
  virtual bool LoadModelAs(const std::filesystem::path& file, const std::string& modelName, const std::string& aliasName, IDirect3DDevice9* device) = 0;
  
  // 獲取已載入的模型列表
  virtual std::vector<std::string> GetLoadedModelNames() const = 0;
  
  // 查詢檔案中包含的模型列表（不實際載入）
  virtual std::vector<std::string> GetAvailableModels(const std::filesystem::path& file) const = 0;
  
  // 檢查模型是否已載入
  virtual bool HasModel(const std::string& name) const noexcept = 0;
  
  virtual const ModelData* GetModel(const std::string& name) const noexcept = 0;
  virtual void Clear() noexcept = 0;
  
  // 移除特定模型
  virtual bool RemoveModel(const std::string& name) = 0;
};

// Factory function
std::unique_ptr<IModelManager> CreateModelManager(std::unique_ptr<IModelLoader> loader, ITextureManager* textureManager);