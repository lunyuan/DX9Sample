#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <d3d9.h>

// Forward declarations
struct ModelData;
struct ID3DXTexture9;

// 資產類型枚舉
enum class AssetType {
    Model,
    Texture,
    Sound,
    Script,
    Config
};

// 資產參考 - 類型安全的句柄
template<typename T>
class AssetRef {
public:
    AssetRef() : id_(""), isValid_(false) {}
    explicit AssetRef(const std::string& id) : id_(id), isValid_(true) {}
    
    const std::string& GetId() const { return id_; }
    bool IsValid() const { return isValid_; }
    
    bool operator==(const AssetRef& other) const { return id_ == other.id_; }
    bool operator!=(const AssetRef& other) const { return !(*this == other); }

private:
    std::string id_;
    bool isValid_;
};

// 資產載入狀態
enum class AssetLoadState {
    NotLoaded,
    Loading,
    Loaded,
    Failed
};

// 資產載入回調
template<typename T>
using AssetLoadCallback = std::function<void(AssetRef<T>, std::shared_ptr<T>, bool success)>;

// 資產管理器介面
struct IAssetManager {
    virtual ~IAssetManager() = default;
    
    // 初始化和配置
    virtual bool Initialize(IDirect3DDevice9* device) = 0;
    virtual void SetAssetRoot(const std::string& rootPath) = 0;
    virtual void SetAssetPath(AssetType type, const std::string& relativePath) = 0;
    
    // 同步載入 - 立即載入並返回資產
    template<typename T>
    std::shared_ptr<T> LoadSync(const std::string& assetPath);
    
    // 異步載入 - 返回 AssetRef，完成後調用回調
    template<typename T>
    AssetRef<T> LoadAsync(const std::string& assetPath, AssetLoadCallback<T> callback = nullptr);
    
    // 獲取已載入的資產
    template<typename T>
    std::shared_ptr<T> Get(const AssetRef<T>& ref);
    
    template<typename T>
    std::shared_ptr<T> Get(const std::string& assetPath);
    
    // 資產狀態查詢
    template<typename T>
    AssetLoadState GetLoadState(const AssetRef<T>& ref);
    
    virtual bool IsLoaded(const std::string& assetPath) const = 0;
    virtual std::vector<std::string> GetLoadedAssets(AssetType type) const = 0;
    
    // 記憶體管理
    virtual void UnloadAsset(const std::string& assetPath) = 0;
    virtual void UnloadUnusedAssets() = 0;
    virtual void UnloadAll() = 0;
    
    // 熱重載支援
    virtual void EnableHotReload(bool enable) = 0;
    virtual void ReloadAsset(const std::string& assetPath) = 0;
    
    // 統計和除錯
    virtual size_t GetMemoryUsage() const = 0;
    virtual size_t GetAssetCount() const = 0;
    virtual void PrintDebugInfo() const = 0;

    // 具體的載入方法 - 避免模板特化問題
    virtual std::shared_ptr<ModelData> LoadModel(const std::string& assetPath) = 0;
    virtual std::vector<std::shared_ptr<ModelData>> LoadAllModels(const std::string& assetPath) = 0;
    virtual std::shared_ptr<IDirect3DTexture9> LoadTexture(const std::string& assetPath) = 0;

protected:
    // 內部實作方法 - 由具體類別實作
    virtual std::shared_ptr<ModelData> LoadModelImpl(const std::string& fullPath) = 0;
    virtual std::shared_ptr<IDirect3DTexture9> LoadTextureImpl(const std::string& fullPath) = 0;
    
    // 解決模板特化的輔助方法
    virtual std::string ResolveAssetPath(const std::string& assetPath, AssetType type) const = 0;
};

// Factory 函式
std::unique_ptr<IAssetManager> CreateAssetManager();