# AssetManager 系統使用指南

## 概述

新的 AssetManager 系統提供了統一的資產管理解決方案，取代了原來硬編碼的資產載入方式。

## 核心功能

### 1. 統一資產管理
- **類型安全**: 支援不同類型的資產 (模型、紋理、音效等)
- **路徑解析**: 自動處理資產路徑和根目錄
- **快取機制**: 避免重複載入相同資產
- **記憶體管理**: 自動清理未使用的資產

### 2. 配置驅動
- **JSON 配置**: 使用 config.json 管理所有資產路徑
- **熱重載**: 支援開發時的資產熱更新
- **多環境**: 支援不同環境的配置切換

## 基本使用方式

### 初始化系統

```cpp
// 1. 建立 AssetManager
auto assetManager = CreateAssetManager();

// 2. 初始化 (需要 D3D 設備)
ComPtr<IDirect3DDevice9> device;
d3dContext_->GetDevice(&device);
assetManager->Initialize(device.Get());

// 3. 設定資產根目錄
assetManager->SetAssetRoot("test/");

// 4. 設定各類型資產的子目錄
assetManager->SetAssetPath(AssetType::Model, "models/");
assetManager->SetAssetPath(AssetType::Texture, "textures/");
assetManager->SetAssetPath(AssetType::Sound, "sounds/");
```

### 載入資產

```cpp
// 載入模型 - 路徑會自動解析為 "test/models/horse_group.x"
auto horseModel = assetManager->LoadModel("horse_group.x");
if (horseModel) {
    std::cout << "模型載入成功!" << std::endl;
}

// 載入紋理 - 路徑會自動解析為 "test/textures/Horse2.bmp"
auto horseTexture = assetManager->LoadTexture("Horse2.bmp");
if (horseTexture) {
    std::cout << "紋理載入成功!" << std::endl;
}

// 使用絕對路徑 (會直接使用，不經過路徑解析)
auto absoluteTexture = assetManager->LoadTexture("C:/path/to/texture.bmp");
```

### 資產狀態查詢

```cpp
// 檢查資產是否已載入
if (assetManager->IsLoaded("horse_group.x")) {
    std::cout << "馬模型已經在記憶體中" << std::endl;
}

// 獲取已載入的所有模型
auto loadedModels = assetManager->GetLoadedAssets(AssetType::Model);
for (const auto& modelPath : loadedModels) {
    std::cout << "已載入模型: " << modelPath << std::endl;
}

// 獲取記憶體使用情況
size_t memUsage = assetManager->GetMemoryUsage();
size_t assetCount = assetManager->GetAssetCount();
std::cout << "已載入 " << assetCount << " 個資產，使用 " << memUsage << " 字節記憶體" << std::endl;
```

### 記憶體管理

```cpp
// 卸載特定資產
assetManager->UnloadAsset("old_model.x");

// 清理未使用的資產 (依據設定的超時時間)
assetManager->UnloadUnusedAssets();

// 卸載所有資產
assetManager->UnloadAll();
```

## 配置系統整合

### 建立 config.json

```json
{
  "engine": {
    "name": "DX9Sample Engine",
    "version": "1.0.0"
  },
  "assets": {
    "rootPath": "test/",
    "modelPath": "models/",
    "texturePath": "textures/",
    "soundPath": "sounds/",
    "scriptPath": "scripts/",
    "configPath": "configs/"
  },
  "graphics": {
    "width": 800,
    "height": 600,
    "fullscreen": false,
    "vsync": true
  },
  "scenes": {
    "defaultScene": "GameScene",
    "menuScene": "MenuScene"
  },
  "ui": {
    "persistentLayers": ["HUD", "Debug"],
    "theme": "default"
  },
  "debug": {
    "enableLogging": true,
    "logLevel": "info",
    "showFPS": true,
    "enableHotReload": false
  }
}
```

### 使用配置系統

```cpp
// 1. 建立配置管理器
auto configManager = CreateConfigManager();

// 2. 載入配置檔案
if (!configManager->LoadConfig("config.json")) {
    std::cerr << "配置載入失敗，使用預設值" << std::endl;
}

// 3. 使用配置初始化 AssetManager
auto assetManager = CreateAssetManager();
assetManager->Initialize(device.Get());

// 從配置讀取路徑設定
std::string assetRoot = configManager->GetString("assets.rootPath", "test/");
assetManager->SetAssetRoot(assetRoot);

assetManager->SetAssetPath(AssetType::Model, 
    configManager->GetString("assets.modelPath", "models/"));
assetManager->SetAssetPath(AssetType::Texture, 
    configManager->GetString("assets.texturePath", "textures/"));

// 啟用熱重載 (如果配置中啟用)
bool enableHotReload = configManager->GetBool("debug.enableHotReload", false);
assetManager->EnableHotReload(enableHotReload);
```

## 在 EngineContext 中的整合範例

### 修改後的 EngineContext.cpp 片段

```cpp
class EngineContext : public IEngineContext {
private:
    std::unique_ptr<IAssetManager> assetManager_;
    std::unique_ptr<IConfigManager> configManager_;
    
public:
    STDMETHODIMP Initialize(HWND hwnd, UINT width, UINT height) {
        // ... 原有的初始化代碼 ...
        
        // 1. 初始化配置系統
        configManager_ = CreateConfigManager();
        if (!configManager_->LoadConfig("config.json")) {
            return E_FAIL;
        }
        
        // 2. 初始化資產管理器
        assetManager_ = CreateAssetManager();
        if (!assetManager_->Initialize(device.Get())) {
            return E_FAIL;
        }
        
        // 3. 從配置設定資產路徑
        SetupAssetPaths();
        
        return S_OK;
    }
    
private:
    void SetupAssetPaths() {
        std::string assetRoot = configManager_->GetString("assets.rootPath", "test/");
        assetManager_->SetAssetRoot(assetRoot);
        
        assetManager_->SetAssetPath(AssetType::Model,
            configManager_->GetString("assets.modelPath", "models/"));
        assetManager_->SetAssetPath(AssetType::Texture,
            configManager_->GetString("assets.texturePath", "textures/"));
        assetManager_->SetAssetPath(AssetType::Sound,
            configManager_->GetString("assets.soundPath", "sounds/"));
    }
    
public:
    STDMETHODIMP LoadAssets(const std::wstring& modelFile, const std::wstring& textureFile) {
        // 轉換為 std::string
        std::string modelPath(modelFile.begin(), modelFile.end());
        std::string texturePath(textureFile.begin(), textureFile.end());
        
        // 使用 AssetManager 載入
        auto model = assetManager_->LoadModel(modelPath);
        auto texture = assetManager_->LoadTexture(texturePath);
        
        if (!model || !texture) {
            return E_FAIL;
        }
        
        // 初始化 Scene3D (可能需要修改 Scene3D 來接受已載入的資產)
        // ...
        
        return S_OK;
    }
};
```

## 開發工具功能

### 除錯資訊

```cpp
// 顯示所有載入的資產和記憶體使用情況
assetManager->PrintDebugInfo();

// 輸出範例:
// === AssetManager Debug Info ===
// Asset Root: test/
// Total Assets: 5
// Memory Usage: 2048576 bytes
// Load Operations: 8
// Hot Reload: Enabled
//
// Loaded Assets:
//   Model [Loaded] test/models/horse_group.x
//   Texture [Loaded] test/textures/Horse2.bmp
//   Texture [Loaded] test/textures/bg.bmp
//   Texture [Loaded] test/textures/bt.bmp
//   Config [Failed] test/configs/invalid.json
// ==============================
```

### 熱重載

```cpp
// 啟用熱重載 (開發模式)
assetManager->EnableHotReload(true);

// 手動重載特定資產
assetManager->ReloadAsset("horse_group.x");
```

## 優勢總結

1. **🎯 解決硬編碼問題**: 資產路徑不再寫死在程式碼中
2. **⚡ 效能提升**: 智能快取避免重複載入
3. **🛠️ 開發友善**: 熱重載支援快速迭代
4. **🧩 模組化**: 清晰的介面分離和責任劃分
5. **📊 可觀測性**: 詳細的除錯資訊和統計
6. **⚙️ 配置化**: JSON 配置支援不同環境設定

這個系統為後續的場景管理和 UI 系統重構奠定了良好的基礎。