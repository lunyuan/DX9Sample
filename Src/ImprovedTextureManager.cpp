#include "ImprovedTextureManager.h"
#include <d3dx9tex.h>
#include <algorithm>
#include <vector>

// Factory 函式
std::unique_ptr<ITextureManager> CreateImprovedTextureManager(IDirect3DDevice9* device) {
    return std::make_unique<ImprovedTextureManager>(device);
}

ImprovedTextureManager::ImprovedTextureManager(IDirect3DDevice9* device)
    : device_(device) {
    if (!device_) {
        throw std::invalid_argument("Device cannot be null");
    }
}

void ImprovedTextureManager::Initialize(ComPtr<IDirect3DDevice9> device) {
    std::unique_lock lock(mutex_);
    device_ = device.Get();
    cache_.clear();
}

ImprovedTextureManager::~ImprovedTextureManager() {
    UnloadAll();
}

std::shared_ptr<IDirect3DBaseTexture9> ImprovedTextureManager::Load(
    const std::filesystem::path& filepath) {
    return LoadWithUsage(filepath, TextureUsage::Model, nullptr);
}

std::shared_ptr<IDirect3DBaseTexture9> ImprovedTextureManager::LoadWithUsage(
    const std::filesystem::path& filepath,
    TextureUsage usage,
    void* user) {
    
    std::string key = filepath.string();
    
    // 嘗試從快取讀取
    {
        std::shared_lock lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            it->second.accessCount++;
            
            if (user) {
                // 升級為寫鎖以添加使用者
                lock.unlock();
                std::unique_lock writeLock(mutex_);
                cache_[key].users.insert(user);
            }
            
            cacheHits_++;
            return it->second.texture;
        }
    }
    
    // 載入新紋理
    cacheMisses_++;
    
    IDirect3DBaseTexture9* rawTexture = nullptr;
    HRESULT hr = LoadTextureWithParams(filepath, usage, &rawTexture);
    
    if (FAILED(hr) || !rawTexture) {
        return nullptr;
    }
    
    // 創建 shared_ptr 與自訂刪除器
    auto deleter = [](IDirect3DBaseTexture9* p) noexcept {
        if (p) p->Release();
    };
    auto texturePtr = std::shared_ptr<IDirect3DBaseTexture9>(rawTexture, deleter);
    
    // 計算記憶體使用量
    size_t memorySize = CalculateTextureMemory(rawTexture);
    
    // 加入快取
    {
        std::unique_lock lock(mutex_);
        
        TextureEntry entry;
        entry.texture = texturePtr;
        entry.usage = usage;
        entry.memorySize = memorySize;
        entry.lastAccess = std::chrono::steady_clock::now();
        entry.accessCount = 1;
        entry.path = filepath;
        
        if (user) {
            entry.users.insert(user);
        }
        
        cache_[key] = std::move(entry);
    }
    
    // 檢查並執行記憶體預算
    EnforceMemoryBudget();
    
    return texturePtr;
}

void ImprovedTextureManager::RegisterUser(
    const std::filesystem::path& filepath, void* user) {
    if (!user) return;
    
    std::unique_lock lock(mutex_);
    auto it = cache_.find(filepath.string());
    if (it != cache_.end()) {
        it->second.users.insert(user);
    }
}

void ImprovedTextureManager::UnregisterUser(void* user) {
    if (!user) return;
    
    std::unique_lock lock(mutex_);
    for (auto& [path, entry] : cache_) {
        entry.users.erase(user);
    }
}

void ImprovedTextureManager::UnregisterUserFromTexture(
    void* user, const std::filesystem::path& filepath) {
    if (!user) return;
    
    std::unique_lock lock(mutex_);
    auto it = cache_.find(filepath.string());
    if (it != cache_.end()) {
        it->second.users.erase(user);
    }
}

void ImprovedTextureManager::SetMemoryBudget(size_t uiBytes, size_t modelBytes) {
    std::unique_lock lock(mutex_);
    uiMemoryBudget_ = uiBytes;
    modelMemoryBudget_ = modelBytes;
}

void ImprovedTextureManager::EnforceMemoryBudget() {
    // 檢查 UI 紋理預算
    while (CalculateUsageMemory(TextureUsage::UI) > uiMemoryBudget_) {
        EvictLeastRecentlyUsed(TextureUsage::UI);
    }
    
    // 檢查 Model 紋理預算
    while (CalculateUsageMemory(TextureUsage::Model) > modelMemoryBudget_) {
        EvictLeastRecentlyUsed(TextureUsage::Model);
    }
}

void ImprovedTextureManager::EvictLeastRecentlyUsed(TextureUsage usage) {
    std::unique_lock lock(mutex_);
    
    // 找出最少使用且無使用者的紋理
    std::string keyToEvict;
    auto oldestTime = std::chrono::steady_clock::now();
    
    for (const auto& [key, entry] : cache_) {
        if (entry.usage == usage && entry.users.empty() && 
            entry.lastAccess < oldestTime) {
            oldestTime = entry.lastAccess;
            keyToEvict = key;
        }
    }
    
    if (!keyToEvict.empty()) {
        cache_.erase(keyToEvict);
    }
}

size_t ImprovedTextureManager::CalculateUsageMemory(TextureUsage usage) const {
    size_t totalMemory = 0;
    for (const auto& [key, entry] : cache_) {
        if (entry.usage == usage) {
            totalMemory += entry.memorySize;
        }
    }
    return totalMemory;
}

void ImprovedTextureManager::PurgeUnusedTextures() {
    std::unique_lock lock(mutex_);
    
    std::vector<std::string> keysToRemove;
    for (const auto& [key, entry] : cache_) {
        if (entry.users.empty()) {
            keysToRemove.push_back(key);
        }
    }
    
    for (const auto& key : keysToRemove) {
        cache_.erase(key);
    }
}

std::shared_ptr<IDirect3DBaseTexture9> ImprovedTextureManager::Get(std::string_view key) const {
    std::shared_lock lock(mutex_);
    auto it = cache_.find(std::string(key));
    if (it != cache_.end()) {
        return it->second.texture;
    }
    return nullptr;
}

void ImprovedTextureManager::Clear() noexcept {
    std::unique_lock lock(mutex_);
    cache_.clear();
}

void ImprovedTextureManager::UnloadAll() {
    Clear();
}

void ImprovedTextureManager::UnloadUnused() {
    PurgeUnusedTextures();
}

size_t ImprovedTextureManager::GetTextureCount() const {
    std::shared_lock lock(mutex_);
    return cache_.size();
}

size_t ImprovedTextureManager::GetMemoryUsage() const {
    std::shared_lock lock(mutex_);
    size_t totalMemory = 0;
    for (const auto& [key, entry] : cache_) {
        totalMemory += entry.memorySize;
    }
    return totalMemory;
}

ImprovedTextureManager::TextureStats ImprovedTextureManager::GetStats() const {
    std::shared_lock lock(mutex_);
    
    TextureStats stats = {};
    stats.totalTextures = cache_.size();
    stats.cacheHits = cacheHits_;
    stats.cacheMisses = cacheMisses_;
    
    for (const auto& [key, entry] : cache_) {
        stats.totalMemory += entry.memorySize;
        
        switch (entry.usage) {
        case TextureUsage::UI:
            stats.uiTextures++;
            stats.uiMemory += entry.memorySize;
            break;
        case TextureUsage::Model:
            stats.modelTextures++;
            stats.modelMemory += entry.memorySize;
            break;
        default:
            break;
        }
    }
    
    return stats;
}

size_t ImprovedTextureManager::CalculateTextureMemory(
    IDirect3DBaseTexture9* texture) const {
    if (!texture) return 0;
    
    D3DRESOURCETYPE type = texture->GetType();
    
    if (type == D3DRTYPE_TEXTURE) {
        auto* tex2D = static_cast<IDirect3DTexture9*>(texture);
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(tex2D->GetLevelDesc(0, &desc))) {
            // 簡化計算：寬 x 高 x 每像素位元組數 x mipmap 層級
            size_t bytesPerPixel = 4; // 假設 32-bit 格式
            size_t mipLevels = tex2D->GetLevelCount();
            
            size_t totalSize = 0;
            for (size_t i = 0; i < mipLevels; ++i) {
                size_t mipWidth = (std::max)(1u, desc.Width >> i);
                size_t mipHeight = (std::max)(1u, desc.Height >> i);
                totalSize += mipWidth * mipHeight * bytesPerPixel;
            }
            
            return totalSize;
        }
    }
    
    return 0; // 無法計算
}

HRESULT ImprovedTextureManager::LoadTextureWithParams(
    const std::filesystem::path& filepath,
    TextureUsage usage,
    IDirect3DBaseTexture9** ppTexture) {
    
    DWORD filter = D3DX_DEFAULT;
    DWORD mipFilter = D3DX_DEFAULT;
    D3DFORMAT format = D3DFMT_UNKNOWN;
    
    // 根據使用類型調整載入參數
    switch (usage) {
    case TextureUsage::UI:
        // UI 紋理：無 mipmap，點過濾
        filter = D3DX_FILTER_NONE;
        mipFilter = D3DX_FILTER_NONE;
        break;
        
    case TextureUsage::Model:
        // Model 紋理：完整 mipmap 鏈，三線性過濾
        filter = D3DX_FILTER_LINEAR;
        mipFilter = D3DX_FILTER_LINEAR;
        break;
        
    case TextureUsage::Shared:
    case TextureUsage::Dynamic:
    default:
        // 使用預設設定
        break;
    }
    
    // 特殊處理：bg.bmp 和 bt.bmp 使用色鍵透明
    std::string filename = filepath.filename().string();
    D3DCOLOR colorKey = 0;
    if (filename == "bg.bmp" || filename == "bt.bmp") {
        colorKey = D3DCOLOR_XRGB(255, 0, 255); // 洋紅色透明
    }
    
    // D3DXCreateTextureFromFileEx 需要 IDirect3DTexture9**
    IDirect3DTexture9* texture = nullptr;
    HRESULT hr = D3DXCreateTextureFromFileEx(
        device_,
        filepath.wstring().c_str(),
        D3DX_DEFAULT,     // Width
        D3DX_DEFAULT,     // Height
        (usage == TextureUsage::UI) ? 1 : D3DX_DEFAULT, // MipLevels
        0,                // Usage
        format,
        D3DPOOL_MANAGED,
        filter,
        mipFilter,
        colorKey,
        nullptr,          // pSrcInfo
        nullptr,          // pPalette
        &texture
    );
    
    if (SUCCEEDED(hr) && texture) {
        *ppTexture = texture;
    }
    
    return hr;
}