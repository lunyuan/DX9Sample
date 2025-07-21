#pragma once

#include "IAssetManager.h"
#include "IModelManager.h"
#include "ITextureManager.h"
#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <future>
#include <thread>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// 資產項目 - 內部使用
struct AssetItem {
    std::string path;
    AssetType type;
    AssetLoadState state;
    std::shared_ptr<void> data;
    size_t refCount;
    std::chrono::time_point<std::chrono::steady_clock> lastAccessed;
    
    AssetItem() : type(AssetType::Model), state(AssetLoadState::NotLoaded), refCount(0) {}
};

class AssetManager : public IAssetManager {
public:
    AssetManager();
    ~AssetManager();
    
    // IAssetManager 介面實作
    bool Initialize(IDirect3DDevice9* device) override;
    void SetAssetRoot(const std::string& rootPath) override;
    void SetAssetPath(AssetType type, const std::string& relativePath) override;
    
    bool IsLoaded(const std::string& assetPath) const override;
    std::vector<std::string> GetLoadedAssets(AssetType type) const override;
    
    void UnloadAsset(const std::string& assetPath) override;
    void UnloadUnusedAssets() override;
    void UnloadAll() override;
    
    void EnableHotReload(bool enable) override;
    void ReloadAsset(const std::string& assetPath) override;
    
    size_t GetMemoryUsage() const override;
    size_t GetAssetCount() const override;
    void PrintDebugInfo() const override;
    
    // 具體載入方法
    std::shared_ptr<ModelData> LoadModel(const std::string& assetPath) override;
    std::vector<std::shared_ptr<ModelData>> LoadAllModels(const std::string& assetPath) override;
    std::shared_ptr<IDirect3DTexture9> LoadTexture(const std::string& assetPath) override;
    
    std::string ResolveAssetPath(const std::string& assetPath, AssetType type) const override;

protected:
    std::shared_ptr<ModelData> LoadModelImpl(const std::string& fullPath) override;
    std::vector<std::shared_ptr<ModelData>> LoadAllModelsImpl(const std::string& fullPath);
    std::shared_ptr<IDirect3DTexture9> LoadTextureImpl(const std::string& fullPath) override;

private:
    // 內部輔助方法
    AssetType DetectAssetType(const std::string& assetPath) const;
    std::string GenerateAssetKey(const std::string& assetPath) const;
    
    // 載入特定類型的資產
    std::shared_ptr<ModelData> LoadModelFromFile(const std::string& fullPath);
    std::shared_ptr<IDirect3DTexture9> LoadTextureFromFile(const std::string& fullPath);
    
    // 記憶體管理
    void UpdateAssetAccess(const std::string& key);
    void CleanupUnusedAssets();
    
    // 熱重載支援
    void StartFileWatcher();
    void StopFileWatcher();
    void OnFileChanged(const std::string& filePath);

private:
    // 核心資料
    IDirect3DDevice9* device_;
    std::string assetRoot_;
    std::unordered_map<AssetType, std::string> assetPaths_;
    
    // 資產快取
    mutable std::shared_mutex assetMutex_;
    std::unordered_map<std::string, AssetItem> assets_;
    
    // 子系統
    std::unique_ptr<IModelManager> modelManager_;
    std::unique_ptr<ITextureManager> textureManager_;
    
    // 熱重載
    bool hotReloadEnabled_;
    std::unique_ptr<std::thread> fileWatcherThread_;
    std::atomic<bool> stopFileWatcher_;
    
    // 統計
    mutable std::atomic<size_t> totalMemoryUsage_;
    mutable std::atomic<size_t> loadOperations_;
    
    // 配置
    size_t maxCacheSize_;
    std::chrono::minutes unusedAssetTimeout_;
};

