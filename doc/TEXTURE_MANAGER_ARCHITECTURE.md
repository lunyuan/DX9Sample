# TextureManager 架構分析與建議

## 現有問題

### 1. 引用計數與生命週期管理
當前 TextureManager 使用 `std::shared_ptr<IDirect3DBaseTexture9>` 提供自動引用計數，但存在以下問題：

- **UI 移除時的紋理清理**：即使 UI 元素被移除，紋理仍可能被快取保留
- **場景切換時的記憶體累積**：沒有明確的快取清理策略
- **共享紋理的追蹤困難**：無法知道哪些 UI/Model 正在使用特定紋理

### 2. UI 顯示 3D Model 的特殊案例
當 UI 需要顯示 3D 模型時（如道具預覽、角色頭像），會產生以下複雜性：

```cpp
// 範例：道具預覽窗口
class ItemPreviewUI {
    // 需要同時訪問 UI 紋理（框架、背景）和 Model 紋理（道具模型）
    ITextureManager* uiTextures;    // UI 專用？
    ITextureManager* modelTextures; // Model 專用？
    // 還是使用單一共享的 TextureManager？
};
```

## 架構方案比較

### 方案 A：單一共享 TextureManager
```cpp
class EngineContext {
    std::shared_ptr<ITextureManager> sharedTextureManager_;
    // UI 和 Model 系統共用
};
```

**優點：**
- 避免重複載入相同紋理
- 簡化架構
- 自動處理共享紋理

**缺點：**
- 快取污染（UI 小圖可能擠掉大型模型紋理）
- 單一鎖競爭點
- 難以實施不同的載入策略

### 方案 B：分離的 TextureManager
```cpp
class EngineContext {
    std::unique_ptr<ITextureManager> uiTextureManager_;
    std::unique_ptr<ITextureManager> modelTextureManager_;
};
```

**優點：**
- 可針對不同用途優化（UI: LRU 快取, Model: 場景快取）
- 降低鎖競爭
- 清晰的職責分離

**缺點：**
- 共享紋理可能重複載入
- 需要協調機制處理跨界使用

### 方案 C：改進的單一 TextureManager（建議方案）
```cpp
enum class TextureUsage {
    UI,          // 小圖，無 mipmap，頻繁存取
    Model,       // 大圖，需要 mipmap，場景生命週期
    Shared,      // 兩者共用
    Dynamic      // 渲染目標
};

class ImprovedTextureManager : public ITextureManager {
    struct TextureEntry {
        std::shared_ptr<IDirect3DBaseTexture9> texture;
        TextureUsage usage;
        std::unordered_set<void*> users;  // 追蹤使用者
        size_t memorySize;
        std::chrono::time_point<std::chrono::steady_clock> lastAccess;
    };
    
    std::unordered_map<std::string, TextureEntry> cache_;
    
public:
    std::shared_ptr<IDirect3DBaseTexture9> Load(
        const std::filesystem::path& path,
        TextureUsage usage = TextureUsage::Model,
        void* user = nullptr
    );
    
    void ReleaseUser(void* user);  // UI 移除時呼叫
    void PurgeUnused();            // 清理未使用的紋理
    void SetMemoryBudget(size_t uiBytes, size_t modelBytes);
};
```

## 實作建議

### 1. 短期改進（維持現有架構）
```cpp
// EngineContext.cpp 修改
HRESULT EngineContext::Initialize(HWND hWnd, UINT width, UINT height) {
    // 使用單一 TextureManager，但加入使用提示
    textureManager_ = CreateImprovedTextureManager(device);
    
    // UI 和 Model 共用，但傳遞使用提示
    uiManager_ = CreateUIManager(textureManager_.get());
    modelManager_ = CreateModelManager(
        std::make_unique<XModelLoader>(), 
        textureManager_.get()
    );
}
```

### 2. UI 移除時的清理策略
```cpp
// UIManager 改進
class UIManager : public IUIManager {
    ~UIManager() {
        // 通知 TextureManager 此 UI 不再使用紋理
        if (textureManager_) {
            textureManager_->ReleaseUser(this);
        }
    }
    
    void RemoveLayer(int layerId) {
        // 移除層級時，檢查並釋放該層的紋理引用
        for (auto& element : GetLayerElements(layerId)) {
            textureManager_->ReleaseUserTexture(this, element.texturePath);
        }
    }
};
```

### 3. 記憶體預算管理
```cpp
class MemoryBudgetedTextureManager {
    size_t uiMemoryBudget_ = 64 * 1024 * 1024;      // 64MB for UI
    size_t modelMemoryBudget_ = 256 * 1024 * 1024;  // 256MB for models
    
    void EnforceMemoryBudget(TextureUsage usage) {
        size_t currentUsage = CalculateUsage(usage);
        size_t budget = (usage == TextureUsage::UI) ? 
                        uiMemoryBudget_ : modelMemoryBudget_;
        
        while (currentUsage > budget) {
            EvictLeastRecentlyUsed(usage);
            currentUsage = CalculateUsage(usage);
        }
    }
};
```

## 結論

建議採用**改進的單一 TextureManager** 方案，原因如下：

1. **保持架構簡潔**：避免過度工程化
2. **解決核心問題**：透過使用者追蹤和記憶體預算管理
3. **支援所有使用案例**：包括 UI 顯示 3D 模型
4. **漸進式改進**：可以逐步實施，不需要大規模重構

關鍵是加入：
- 使用者追蹤機制
- 分類的記憶體預算
- 明確的生命週期管理
- 使用提示優化載入參數

這樣可以在保持簡單架構的同時，解決紋理管理的實際問題。