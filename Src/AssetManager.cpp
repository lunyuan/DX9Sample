#include "AssetManager.h"
#include <iostream>
#include "ModelManager.h"
#include "TextureManager.h"
#include "XModelLoader.h"
#include "ModelData.h"
#include <d3dx9.h>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// Factory 函式實作
std::unique_ptr<IAssetManager> CreateAssetManager() {
    return std::make_unique<AssetManager>();
}

AssetManager::AssetManager() 
    : device_(nullptr)
    , assetRoot_("./")
    , hotReloadEnabled_(false)
    , stopFileWatcher_(false)
    , totalMemoryUsage_(0)
    , loadOperations_(0)
    , maxCacheSize_(100)
    , unusedAssetTimeout_(std::chrono::minutes(5))
{
    // 設定預設的資產路徑
    assetPaths_[AssetType::Model] = "models/";
    assetPaths_[AssetType::Texture] = "textures/";
    assetPaths_[AssetType::Sound] = "sounds/";
    assetPaths_[AssetType::Script] = "scripts/";
    assetPaths_[AssetType::Config] = "configs/";
}

AssetManager::~AssetManager() {
    StopFileWatcher();
    UnloadAll();
}

bool AssetManager::Initialize(IDirect3DDevice9* device) {
    if (!device) {
        std::cerr << "AssetManager::Initialize: Invalid device" << std::endl;
        return false;
    }
    
    device_ = device;
    
    // 初始化子系統
    textureManager_ = CreateTextureManager(device);
    if (!textureManager_) {
        std::cerr << "AssetManager: Failed to create TextureManager" << std::endl;
        return false;
    }
    
    modelManager_ = CreateModelManager(std::make_unique<XModelLoader>(), textureManager_.get());
    if (!modelManager_) {
        std::cerr << "AssetManager: Failed to create ModelManager" << std::endl;
        return false;
    }
    
    return true;
}

void AssetManager::SetAssetRoot(const std::string& rootPath) {
    std::lock_guard<std::shared_mutex> lock(assetMutex_);
    assetRoot_ = rootPath;
    
    // 確保路徑以 / 結尾
    if (!assetRoot_.empty() && assetRoot_.back() != '/' && assetRoot_.back() != '\\') {
        assetRoot_ += "/";
    }
    
}

void AssetManager::SetAssetPath(AssetType type, const std::string& relativePath) {
    std::lock_guard<std::shared_mutex> lock(assetMutex_);
    assetPaths_[type] = relativePath;
    
    // 確保路徑以 / 結尾
    if (!assetPaths_[type].empty() && 
        assetPaths_[type].back() != '/' && 
        assetPaths_[type].back() != '\\') {
        assetPaths_[type] += "/";
    }
}

std::string AssetManager::ResolveAssetPath(const std::string& assetPath, AssetType type) const {
    std::shared_lock<std::shared_mutex> lock(assetMutex_);
    
    // 如果是絕對路徑，直接使用
    if (fs::path(assetPath).is_absolute()) {
        return assetPath;
    }
    
    // 建構完整路徑: assetRoot + typePath + assetPath
    std::string fullPath = assetRoot_;
    
    auto it = assetPaths_.find(type);
    if (it != assetPaths_.end()) {
        fullPath += it->second;
    }
    
    fullPath += assetPath;
    
    return fullPath;
}

AssetType AssetManager::DetectAssetType(const std::string& assetPath) const {
    std::string ext = fs::path(assetPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".x" || ext == ".fbx" || ext == ".gltf") {
        return AssetType::Model;
    } else if (ext == ".bmp" || ext == ".jpg" || ext == ".jpeg" || 
               ext == ".png" || ext == ".dds" || ext == ".tga") {
        return AssetType::Texture;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::Sound;
    } else if (ext == ".json" || ext == ".xml" || ext == ".ini") {
        return AssetType::Config;
    } else {
        return AssetType::Script; // 預設
    }
}

std::string AssetManager::GenerateAssetKey(const std::string& assetPath) const {
    // 使用正規化的路徑作為鍵值
    return fs::path(assetPath).lexically_normal().string();
}

std::shared_ptr<ModelData> AssetManager::LoadModelImpl(const std::string& fullPath) {
    std::string key = GenerateAssetKey(fullPath);
    
    // 檢查快取
    {
        std::shared_lock<std::shared_mutex> lock(assetMutex_);
        auto it = assets_.find(key);
        if (it != assets_.end() && it->second.state == AssetLoadState::Loaded) {
            UpdateAssetAccess(key);
            return std::static_pointer_cast<ModelData>(it->second.data);
        }
    }
    
    // 載入模型
    try {
        std::wstring wPath(fullPath.begin(), fullPath.end());
        
        // 使用 ModelManager 載入
        modelManager_->LoadModels(wPath, device_);
        auto modelNames = modelManager_->GetLoadedModelNames();
        
        if (!modelNames.empty()) {
            // 取得第一個模型
            const ModelData* modelPtr = modelManager_->GetModel(modelNames[0]);
            if (modelPtr) {
                auto modelData = std::make_shared<ModelData>(*modelPtr);
                
                // 加入快取
                {
                    std::lock_guard<std::shared_mutex> lock(assetMutex_);
                    AssetItem& item = assets_[key];
                    item.path = fullPath;
                    item.type = AssetType::Model;
                    item.state = AssetLoadState::Loaded;
                    item.data = modelData;
                    item.refCount = 1;
                    item.lastAccessed = std::chrono::steady_clock::now();
                }
                
                loadOperations_++;
                return modelData;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load model " << fullPath << ": " << e.what() << std::endl;
        
        // 標記為載入失敗
        {
            std::lock_guard<std::shared_mutex> lock(assetMutex_);
            AssetItem& item = assets_[key];
            item.path = fullPath;
            item.type = AssetType::Model;
            item.state = AssetLoadState::Failed;
        }
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<ModelData>> AssetManager::LoadAllModelsImpl(const std::string& fullPath) {
    std::vector<std::shared_ptr<ModelData>> result;
    std::string baseKey = GenerateAssetKey(fullPath);
    
    try {
        std::wstring wPath(fullPath.begin(), fullPath.end());
        
        // 使用 ModelManager 載入所有模型
        modelManager_->LoadModels(wPath, device_);
        auto modelNames = modelManager_->GetLoadedModelNames();
        
        OutputDebugStringA(("LoadAllModelsImpl: Found " + std::to_string(modelNames.size()) + " models in " + fullPath + "\n").c_str());
        
        for (const auto& modelName : modelNames) {
            const ModelData* modelPtr = modelManager_->GetModel(modelName);
            if (modelPtr) {
                auto modelData = std::make_shared<ModelData>(*modelPtr);
                result.push_back(modelData);
                
                // 加入快取，使用模型名稱作為鍵值的一部分
                std::string key = baseKey + "::" + modelName;
                {
                    std::lock_guard<std::shared_mutex> lock(assetMutex_);
                    AssetItem& item = assets_[key];
                    item.path = fullPath + "::" + modelName;
                    item.type = AssetType::Model;
                    item.state = AssetLoadState::Loaded;
                    item.data = modelData;
                    item.refCount = 1;
                    item.lastAccessed = std::chrono::steady_clock::now();
                }
                
                OutputDebugStringA(("  - Loaded model: " + modelName + "\n").c_str());
            }
        }
        
        loadOperations_++;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load models from " << fullPath << ": " << e.what() << std::endl;
        OutputDebugStringA(("LoadAllModelsImpl error: " + std::string(e.what()) + "\n").c_str());
    }
    
    return result;
}

std::shared_ptr<IDirect3DTexture9> AssetManager::LoadTextureImpl(const std::string& fullPath) {
    std::string key = GenerateAssetKey(fullPath);
    
    // 檢查快取
    {
        std::shared_lock<std::shared_mutex> lock(assetMutex_);
        auto it = assets_.find(key);
        if (it != assets_.end() && it->second.state == AssetLoadState::Loaded) {
            UpdateAssetAccess(key);
            return std::static_pointer_cast<IDirect3DTexture9>(it->second.data);
        }
    }
    
    // 載入紋理
    try {
        std::filesystem::path fsPath(fullPath);
        auto texture = textureManager_->Load(fsPath);
        
        if (texture) {
            // 加入快取
            {
                std::lock_guard<std::shared_mutex> lock(assetMutex_);
                AssetItem& item = assets_[key];
                item.path = fullPath;
                item.type = AssetType::Texture;
                item.state = AssetLoadState::Loaded;
                item.data = texture;
                item.refCount = 1;
                item.lastAccessed = std::chrono::steady_clock::now();
            }
            
            loadOperations_++;
            return std::static_pointer_cast<IDirect3DTexture9>(texture);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load texture " << fullPath << ": " << e.what() << std::endl;
        
        // 標記為載入失敗
        {
            std::lock_guard<std::shared_mutex> lock(assetMutex_);
            AssetItem& item = assets_[key];
            item.path = fullPath;
            item.type = AssetType::Texture;
            item.state = AssetLoadState::Failed;
        }
    }
    
    return nullptr;
}

bool AssetManager::IsLoaded(const std::string& assetPath) const {
    AssetType type = DetectAssetType(assetPath);
    std::string fullPath = ResolveAssetPath(assetPath, type);
    std::string key = GenerateAssetKey(fullPath);
    
    std::shared_lock<std::shared_mutex> lock(assetMutex_);
    auto it = assets_.find(key);
    return it != assets_.end() && it->second.state == AssetLoadState::Loaded;
}

std::vector<std::string> AssetManager::GetLoadedAssets(AssetType type) const {
    std::shared_lock<std::shared_mutex> lock(assetMutex_);
    std::vector<std::string> result;
    
    for (const auto& pair : assets_) {
        if (pair.second.type == type && pair.second.state == AssetLoadState::Loaded) {
            result.push_back(pair.second.path);
        }
    }
    
    return result;
}

void AssetManager::UnloadAsset(const std::string& assetPath) {
    AssetType type = DetectAssetType(assetPath);
    std::string fullPath = ResolveAssetPath(assetPath, type);
    std::string key = GenerateAssetKey(fullPath);
    
    std::lock_guard<std::shared_mutex> lock(assetMutex_);
    auto it = assets_.find(key);
    if (it != assets_.end()) {
        assets_.erase(it);
    }
}

void AssetManager::UnloadUnusedAssets() {
    std::lock_guard<std::shared_mutex> lock(assetMutex_);
    auto now = std::chrono::steady_clock::now();
    
    auto it = assets_.begin();
    while (it != assets_.end()) {
        auto timeSinceAccess = now - it->second.lastAccessed;
        if (timeSinceAccess > unusedAssetTimeout_ && it->second.refCount == 0) {
            it = assets_.erase(it);
        } else {
            ++it;
        }
    }
}

void AssetManager::UnloadAll() {
    std::lock_guard<std::shared_mutex> lock(assetMutex_);
    assets_.clear();
    totalMemoryUsage_ = 0;
}

void AssetManager::EnableHotReload(bool enable) {
    if (enable != hotReloadEnabled_) {
        hotReloadEnabled_ = enable;
        
        if (enable) {
            StartFileWatcher();
        } else {
            StopFileWatcher();
        }
    }
}

void AssetManager::ReloadAsset(const std::string& assetPath) {
    // 卸載現有資產
    UnloadAsset(assetPath);
    
    // 重新載入
    AssetType type = DetectAssetType(assetPath);
    if (type == AssetType::Model) {
        LoadModelImpl(ResolveAssetPath(assetPath, type));
    } else if (type == AssetType::Texture) {
        LoadTextureImpl(ResolveAssetPath(assetPath, type));
    }
}

size_t AssetManager::GetMemoryUsage() const {
    return totalMemoryUsage_;
}

size_t AssetManager::GetAssetCount() const {
    std::shared_lock<std::shared_mutex> lock(assetMutex_);
    return assets_.size();
}

void AssetManager::PrintDebugInfo() const {
    std::shared_lock<std::shared_mutex> lock(assetMutex_);
    
    // AssetManager debug info removed for minimal logging
    for (const auto& pair : assets_) {
        const AssetItem& item = pair.second;
        std::string typeStr;
        switch (item.type) {
            case AssetType::Model: typeStr = "Model"; break;
            case AssetType::Texture: typeStr = "Texture"; break;
            case AssetType::Sound: typeStr = "Sound"; break;
            case AssetType::Script: typeStr = "Script"; break;
            case AssetType::Config: typeStr = "Config"; break;
        }
        
        std::string stateStr;
        switch (item.state) {
            case AssetLoadState::NotLoaded: stateStr = "NotLoaded"; break;
            case AssetLoadState::Loading: stateStr = "Loading"; break;
            case AssetLoadState::Loaded: stateStr = "Loaded"; break;
            case AssetLoadState::Failed: stateStr = "Failed"; break;
        }
        
    }
}

void AssetManager::UpdateAssetAccess(const std::string& key) {
    // 注意：這個方法假設調用者已經有鎖
    auto it = assets_.find(key);
    if (it != assets_.end()) {
        it->second.lastAccessed = std::chrono::steady_clock::now();
        it->second.refCount++;
    }
}

void AssetManager::StartFileWatcher() {
    // 簡化實作：實際應用中可以使用 Windows API 或跨平台庫來監控檔案變化
    // 這裡只是一個示範
    stopFileWatcher_ = false;
    fileWatcherThread_ = std::make_unique<std::thread>([this]() {
        while (!stopFileWatcher_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // 在實際實作中，這裡會檢查檔案變化
        }
    });
    
}

void AssetManager::StopFileWatcher() {
    if (fileWatcherThread_) {
        stopFileWatcher_ = true;
        if (fileWatcherThread_->joinable()) {
            fileWatcherThread_->join();
        }
        fileWatcherThread_.reset();
    }
}

void AssetManager::OnFileChanged(const std::string& filePath) {
    // 檔案變化處理
    ReloadAsset(filePath);
}

// 公開的載入方法實作
std::shared_ptr<ModelData> AssetManager::LoadModel(const std::string& assetPath) {
    std::string fullPath = ResolveAssetPath(assetPath, AssetType::Model);
    return LoadModelImpl(fullPath);
}

std::vector<std::shared_ptr<ModelData>> AssetManager::LoadAllModels(const std::string& assetPath) {
    std::string fullPath = ResolveAssetPath(assetPath, AssetType::Model);
    return LoadAllModelsImpl(fullPath);
}

std::shared_ptr<IDirect3DTexture9> AssetManager::LoadTexture(const std::string& assetPath) {
    std::string fullPath = ResolveAssetPath(assetPath, AssetType::Texture);
    return LoadTextureImpl(fullPath);
}