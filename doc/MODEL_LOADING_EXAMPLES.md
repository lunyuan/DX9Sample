# 模型載入系統使用範例

## 新的 ModelManager 功能

經過重構後，ModelManager 現在支援更靈活的模型載入方式：

### 1. 載入檔案中的所有模型 (原有功能)
```cpp
// 載入 horse_group.x 中的所有模型
modelManager_->LoadModels(L"test/horse_group.x", device.Get());

// 獲取載入的模型列表
auto loadedModels = modelManager_->GetLoadedModelNames();
for (const auto& name : loadedModels) {
    std::cout << "已載入模型: " << name << std::endl;
}
```

### 2. 查詢檔案中包含的模型列表 (不實際載入)
```cpp
// 查詢檔案中有哪些模型，但不載入
auto availableModels = modelManager_->GetAvailableModels(L"test/horse_group.x");
for (const auto& name : availableModels) {
    std::cout << "可用模型: " << name << std::endl;
}
```

### 3. 載入特定模型
```cpp
// 只載入檔案中的特定模型
bool success = modelManager_->LoadModel(L"test/horse_group.x", "Horse1", device.Get());
if (success) {
    std::cout << "成功載入 Horse1 模型" << std::endl;
} else {
    std::cout << "找不到 Horse1 模型" << std::endl;
}
```

### 4. 載入模型並重新命名
```cpp
// 載入模型並給予新的名稱
bool success = modelManager_->LoadModelAs(
    L"test/horse_group.x", 
    "Horse1",           // 檔案中的模型名稱
    "PlayerHorse",      // 新的別名
    device.Get()
);

if (success) {
    // 使用新名稱獲取模型
    const ModelData* model = modelManager_->GetModel("PlayerHorse");
}
```

### 5. 模型管理功能
```cpp
// 檢查模型是否已載入
if (modelManager_->HasModel("PlayerHorse")) {
    std::cout << "PlayerHorse 已載入" << std::endl;
}

// 移除特定模型
modelManager_->RemoveModel("PlayerHorse");

// 清除所有模型
modelManager_->Clear();
```

## 實際使用場景範例

### 場景 1: 動態載入不同角色
```cpp
void LoadCharacter(const std::string& characterType) {
    std::wstring fileName = L"models/" + std::wstring(characterType.begin(), characterType.end()) + L".x";
    
    // 查詢角色檔案中有哪些模型
    auto models = modelManager_->GetAvailableModels(fileName);
    
    if (!models.empty()) {
        // 載入第一個模型作為主角色模型
        modelManager_->LoadModelAs(fileName, models[0], "MainCharacter", device.Get());
        
        // 如果有多個模型，載入其他部分
        for (size_t i = 1; i < models.size(); ++i) {
            std::string partName = "CharacterPart" + std::to_string(i);
            modelManager_->LoadModelAs(fileName, models[i], partName, device.Get());
        }
    }
}
```

### 場景 2: 載入場景中的特定物件
```cpp
void LoadSceneObjects() {
    // 查詢場景檔案中有哪些物件
    auto objects = modelManager_->GetAvailableModels(L"scenes/level1.x");
    
    // 只載入需要的物件
    std::vector<std::string> requiredObjects = {"Tree1", "House", "Fence"};
    
    for (const auto& objName : requiredObjects) {
        if (std::find(objects.begin(), objects.end(), objName) != objects.end()) {
            modelManager_->LoadModel(L"scenes/level1.x", objName, device.Get());
        }
    }
}
```

### 場景 3: 模型實例管理
```cpp
void CreateMultipleHorses() {
    // 載入基礎馬模型
    modelManager_->LoadModel(L"test/horse_group.x", "Horse1", device.Get());
    
    // 為不同的馬實例創建別名
    for (int i = 0; i < 5; ++i) {
        std::string horseName = "Horse" + std::to_string(i);
        modelManager_->LoadModelAs(L"test/horse_group.x", "Horse1", horseName, device.Get());
    }
}
```

## 錯誤處理建議

```cpp
void SafeModelLoading() {
    try {
        // 先查詢可用模型
        auto models = modelManager_->GetAvailableModels(L"test/horse_group.x");
        
        if (models.empty()) {
            std::cerr << "檔案中沒有找到任何模型" << std::endl;
            return;
        }
        
        // 嘗試載入特定模型
        if (!modelManager_->LoadModel(L"test/horse_group.x", "Horse1", device.Get())) {
            std::cerr << "無法載入指定的模型" << std::endl;
            
            // 載入第一個可用的模型作為備選
            if (!models.empty()) {
                modelManager_->LoadModel(L"test/horse_group.x", models[0], device.Get());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "模型載入錯誤: " << e.what() << std::endl;
    }
}
```

## 與現有程式碼的整合

在 EngineContext.cpp 中，您可以這樣使用新功能：

```cpp
STDMETHODIMP EngineContext::LoadAssets(
  const std::wstring& modelFile,
  const std::wstring& textureFile) {
  
  // 取得 DirectX 裝置
  ComPtr<IDirect3DDevice9> device;
  HRESULT hr = d3dContext_->GetDevice(&device);
  if (FAILED(hr)) return hr;
  
  // 查詢檔案中有哪些模型
  auto availableModels = modelManager_->GetAvailableModels(modelFile);
  
  if (availableModels.empty()) {
    return E_FAIL; // 檔案中沒有模型
  }
  
  // 載入第一個模型作為主要模型
  if (!modelManager_->LoadModel(modelFile, availableModels[0], device.Get())) {
    return E_FAIL;
  }
  
  // 載入到 Scene3D...
  hr = scene3D_->Init(device.Get(), lightManager_.get(), modelFile, textureFile);
  if (FAILED(hr)) return hr;
  
  return S_OK;
}
```

這個新的系統提供了更大的靈活性，讓您可以精確控制載入哪些模型，並且避免載入不需要的資源。