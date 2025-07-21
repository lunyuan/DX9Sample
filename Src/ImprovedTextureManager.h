#pragma once

#include "ITextureManager.h"
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <shared_mutex>
#include <queue>

enum class TextureUsage {
    UI,          // 小圖，無 mipmap，頻繁存取
    Model,       // 大圖，需要 mipmap，場景生命週期
    Shared,      // 兩者共用
    Dynamic      // 渲染目標
};

// 改進的 TextureManager，支援使用追蹤和記憶體管理
class ImprovedTextureManager : public ITextureManager {
public:
    explicit ImprovedTextureManager(IDirect3DDevice9* device);
    ~ImprovedTextureManager() override;

    // ITextureManager 介面
    void Initialize(ComPtr<IDirect3DDevice9> device) override;
    std::shared_ptr<IDirect3DBaseTexture9> Load(const std::filesystem::path& filepath) override;
    std::shared_ptr<IDirect3DBaseTexture9> Get(std::string_view key) const override;
    void Clear() noexcept override;

    // 擴充功能
    std::shared_ptr<IDirect3DBaseTexture9> LoadWithUsage(
        const std::filesystem::path& filepath,
        TextureUsage usage = TextureUsage::Model,
        void* user = nullptr
    );

    // 使用者管理
    void RegisterUser(const std::filesystem::path& filepath, void* user);
    void UnregisterUser(void* user);
    void UnregisterUserFromTexture(void* user, const std::filesystem::path& filepath);

    // 記憶體預算管理
    void SetMemoryBudget(size_t uiBytes, size_t modelBytes);
    void EnforceMemoryBudget();

    // 擴充功能 - 不在基礎介面中
    void UnloadAll();
    void UnloadUnused();
    size_t GetTextureCount() const;
    size_t GetMemoryUsage() const;

    // 統計資訊
    struct TextureStats {
        size_t totalTextures;
        size_t uiTextures;
        size_t modelTextures;
        size_t totalMemory;
        size_t uiMemory;
        size_t modelMemory;
        size_t cacheHits;
        size_t cacheMisses;
    };
    TextureStats GetStats() const;

private:
    struct TextureEntry {
        std::shared_ptr<IDirect3DBaseTexture9> texture;
        TextureUsage usage;
        std::unordered_set<void*> users;
        size_t memorySize;
        std::chrono::steady_clock::time_point lastAccess;
        size_t accessCount;
        std::filesystem::path path;
    };

    // 計算紋理記憶體大小
    size_t CalculateTextureMemory(IDirect3DBaseTexture9* texture) const;

    // 根據使用類型決定載入參數
    HRESULT LoadTextureWithParams(
        const std::filesystem::path& filepath,
        TextureUsage usage,
        IDirect3DBaseTexture9** ppTexture
    );

    // 記憶體管理
    void EvictLeastRecentlyUsed(TextureUsage usage);
    size_t CalculateUsageMemory(TextureUsage usage) const;

    // 清理無使用者的紋理
    void PurgeUnusedTextures();

private:
    IDirect3DDevice9* device_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, TextureEntry> cache_;

    // 記憶體預算
    size_t uiMemoryBudget_ = 64 * 1024 * 1024;      // 64MB for UI
    size_t modelMemoryBudget_ = 256 * 1024 * 1024;  // 256MB for models

    // 統計
    mutable size_t cacheHits_ = 0;
    mutable size_t cacheMisses_ = 0;
};

// Factory 函式
std::unique_ptr<ITextureManager> CreateImprovedTextureManager(IDirect3DDevice9* device);