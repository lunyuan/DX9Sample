# DX9Sample API 參考文件

**版本**: 2.0.0  
**最後更新**: 2025-07-23

## 目錄

1. [核心介面](#核心介面)
2. [資源管理](#資源管理)
3. [渲染系統](#渲染系統)
4. [UI 系統](#ui-系統)
5. [事件系統](#事件系統)
6. [場景管理](#場景管理)
7. [輸入處理](#輸入處理)
8. [工廠函式](#工廠函式)

---

## 核心介面

### IEngineContext

引擎核心介面，管理所有子系統的生命週期。

```cpp
struct IEngineContext {
    virtual ~IEngineContext() = default;
    
    // 初始化引擎
    virtual HRESULT Initialize(HWND hwnd, UINT width, UINT height) = 0;
    
    // 載入初始資源
    virtual HRESULT LoadAssets(const std::wstring& modelFile, 
                              const std::wstring& textureFile) = 0;
    
    // 執行主循環
    virtual HRESULT Run() = 0;
    
    // 取得子系統
    virtual ITextureManager* GetTextureManager() = 0;
    virtual IEffectManager* GetEffectManager() = 0;
    virtual ID3DContext* GetD3DContext() = 0;
    virtual IModelManager* GetModelManager() = 0;
    virtual ILightManager* GetLightManager() = 0;
    virtual IScene3D* GetScene3D() = 0;
    virtual IUIManager* GetUIManager() = 0;
    virtual IInputHandler* GetInputHandler() = 0;
    virtual ICameraController* GetCameraController() = 0;
    virtual IFullScreenQuad* GetPostProcessor() = 0;
    
    // 現代架構系統
    virtual ISceneManager* GetSceneManager() = 0;
    virtual IAssetManager* GetAssetManager() = 0;
    virtual IEventManager* GetEventManager() = 0;
    virtual IConfigManager* GetConfigManager() = 0;
    virtual IServiceLocator* GetServices() = 0;
};

// 工廠函式
std::unique_ptr<IEngineContext> CreateEngineContext();
```

**使用範例**:
```cpp
auto engine = CreateEngineContext();
HRESULT hr = engine->Initialize(hwnd, 1280, 720);
if (SUCCEEDED(hr)) {
    engine->Run();
}
```

### IServiceLocator

服務定位器介面，提供依賴注入。

```cpp
struct IServiceLocator {
    virtual ~IServiceLocator() = default;
    
    // 現代架構服務
    virtual IAssetManager* GetAssetManager() const = 0;
    virtual IUIManager* GetUIManager() const = 0;
    virtual IEventManager* GetEventManager() const = 0;
    virtual IConfigManager* GetConfigManager() const = 0;
    virtual ISceneManager* GetSceneManager() const = 0;
    virtual IDirect3DDevice9* GetDevice() const = 0;
    virtual ICameraController* GetCameraController() const = 0;
    
    // 檢查服務是否有效
    virtual bool IsValid() const = 0;
};
```

---

## 資源管理

### IAssetManager

統一的資源管理介面。

```cpp
struct IAssetManager {
    virtual ~IAssetManager() = default;
    
    // 初始化
    virtual bool Initialize(IDirect3DDevice9* device) = 0;
    virtual void SetAssetRoot(const std::string& rootPath) = 0;
    virtual void SetAssetPath(AssetType type, const std::string& relativePath) = 0;
    
    // 模型載入
    virtual std::shared_ptr<ModelData> LoadModel(const std::string& assetPath) = 0;
    virtual std::vector<std::shared_ptr<ModelData>> LoadAllModels(
        const std::string& assetPath) = 0;
    
    // 紋理載入
    virtual std::shared_ptr<IDirect3DTexture9> LoadTexture(
        const std::string& assetPath) = 0;
    
    // 資源管理
    virtual bool IsLoaded(const std::string& assetPath) const = 0;
    virtual void UnloadAsset(const std::string& assetPath) = 0;
    virtual void UnloadUnusedAssets() = 0;
    virtual void UnloadAll() = 0;
    
    // 熱重載
    virtual void EnableHotReload(bool enable) = 0;
    virtual void ReloadAsset(const std::string& assetPath) = 0;
    
    // 統計
    virtual size_t GetMemoryUsage() const = 0;
    virtual size_t GetAssetCount() const = 0;
};

// 工廠函式
std::unique_ptr<IAssetManager> CreateAssetManager();
```

**支援的檔案格式**:
- 模型: `.x`, `.fbx`, `.gltf`
- 紋理: `.bmp`, `.jpg`, `.png`, `.dds`, `.tga`

### ITextureManager

紋理管理介面，提供執行緒安全的紋理快取。

```cpp
struct ITextureManager {
    virtual ~ITextureManager() = default;
    
    // 載入紋理（帶快取）
    virtual HRESULT LoadTexture(const std::wstring& filename,
                               IDirect3DTexture9** ppTexture) = 0;
    
    // 取得紋理（從快取）
    virtual IDirect3DTexture9* GetTexture(const std::wstring& filename) = 0;
    
    // 快取管理
    virtual void ReleaseTexture(const std::wstring& filename) = 0;
    virtual void ClearCache() = 0;
    virtual size_t GetCacheSize() const = 0;
    
    // 紋理資訊
    virtual bool GetTextureInfo(const std::wstring& filename,
                               D3DXIMAGE_INFO* pInfo) = 0;
};

// 工廠函式
std::unique_ptr<ITextureManager> CreateTextureManager(
    ComPtr<IDirect3DDevice9> device);
```

---

## 渲染系統

### ID3DContext

Direct3D 裝置管理介面。

```cpp
struct ID3DContext {
    virtual ~ID3DContext() = default;
    
    // 初始化
    virtual HRESULT Init(HWND hwnd, UINT width, UINT height,
                        D3DDEVTYPE devType, DWORD createFlags) = 0;
    
    // 裝置存取
    virtual HRESULT GetDevice(IDirect3DDevice9** ppDev) = 0;
    
    // 渲染控制
    virtual HRESULT Clear(DWORD flags, D3DCOLOR color, 
                         float z, DWORD stencil) = 0;
    virtual HRESULT BeginScene() = 0;
    virtual HRESULT EndScene() = 0;
    virtual HRESULT Present() = 0;
    
    // 裝置狀態
    virtual HRESULT CheckDeviceState() = 0;
    virtual HRESULT ResetDevice() = 0;
};

// 工廠函式
std::unique_ptr<ID3DContext> CreateD3DContext();
```

### IScene3D

3D 場景渲染介面。

```cpp
struct IScene3D {
    virtual ~IScene3D() = default;
    
    // 初始化場景
    virtual HRESULT Init(IDirect3DDevice9* dev,
                        ILightManager* lightMgr,
                        const std::wstring& meshFile,
                        const std::wstring& texFile) = 0;
    
    // 渲染
    virtual void Render(IDirect3DDevice9* device,
                       const D3DXMATRIX& view,
                       const D3DXMATRIX& proj,
                       IUIManager* uiMgr) = 0;
};

// 工廠函式
std::unique_ptr<IScene3D> CreateScene3D();
```

---

## UI 系統

### IUIManager

UI 管理器介面。

```cpp
struct IUIManager {
    virtual ~IUIManager() = default;
    
    // 初始化
    virtual HRESULT Init(IDirect3DDevice9* dev) = 0;
    virtual HRESULT Render(IDirect3DDevice9* dev) = 0;
    
    // 輸入處理
    virtual bool HandleMessage(const MSG& msg) = 0;
    
    // 層級管理
    virtual int CreateLayer(const std::wstring& name, 
                           float minZ, float maxZ) = 0;
    virtual void SetActiveLayer(int layerId) = 0;
    
    // 元件管理
    virtual void AddComponent(std::unique_ptr<UIComponentNew> component) = 0;
    virtual void RemoveComponent(int id) = 0;
    virtual UIComponentNew* FindComponentByName(const std::wstring& name) = 0;
    virtual UIComponentNew* FindComponentById(int id) = 0;
    
    // 文字管理
    virtual int AddText(const std::wstring& text, int x, int y,
                       D3DCOLOR color = 0xFFFFFFFF) = 0;
    virtual void UpdateText(int id, const std::wstring& newText) = 0;
    virtual void RemoveText(int id) = 0;
    
    // UI 創建輔助函式
    virtual UIComponentNew* CreateImage(const std::wstring& imagePath,
                                       int x, int y, int width, int height,
                                       DragMode dragMode = DragMode::None,
                                       UIComponentNew* parent = nullptr,
                                       const std::wstring& name = L"") = 0;
    
    virtual UIComponentNew* CreateButton(const std::wstring& text,
                                        int x, int y, int width, int height,
                                        std::function<void()> onClick,
                                        UIComponentNew* parent = nullptr,
                                        const std::wstring& name = L"") = 0;
};

// 工廠函式
std::unique_ptr<IUIManager> CreateUIManager(ITextureManager* textureManager = nullptr);
```

### UIComponentNew

UI 元件基類。

```cpp
struct UIComponentNew {
    int id = -1;
    std::wstring name;
    
    // 位置和大小
    int relativeX = 0, relativeY = 0;
    int absoluteX = 0, absoluteY = 0;
    int width = 0, height = 0;
    
    // 狀態
    bool visible = true;
    bool enabled = true;
    DragMode dragMode = DragMode::None;
    
    // 層級關係
    std::vector<std::unique_ptr<UIComponentNew>> children;
    UIComponentNew* parent = nullptr;
    UIManager* manager = nullptr;
    
    // 虛擬函式
    virtual ComponentType GetType() const = 0;
    virtual void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite,
                       ITextureManager* texMgr, ID3DXFont* font = nullptr) = 0;
    
    // 輸入處理
    virtual bool OnMouseDown(int x, int y, bool isRightButton) { return false; }
    virtual bool OnMouseUp(int x, int y, bool isRightButton) { return false; }
    virtual bool OnMouseMove(int x, int y) { return false; }
    virtual bool OnKeyDown(WPARAM key) { return false; }
    virtual bool OnChar(WPARAM ch) { return false; }
    
    // 拖放
    virtual void OnDragStart() {}
    virtual void OnDragEnd(bool cancelled) {}
    virtual void OnDragEnter(UIComponentNew* dragged) {}
    virtual void OnDragLeave(UIComponentNew* dragged) {}
    virtual bool OnDrop(UIComponentNew* dragged) { return false; }
};
```

---

## 事件系統

### IEventManager

類型安全的事件管理器。

```cpp
struct IEventManager {
    virtual ~IEventManager() = default;
    
    // 發布事件（立即）
    template<typename T>
    void Publish(const T& event);
    
    // 佇列事件（延遲處理）
    template<typename T>
    void QueueEvent(std::unique_ptr<T> event);
    
    // 訂閱事件
    template<typename T>
    void Subscribe(std::function<void(const T&)> handler);
    
    // 取消訂閱
    template<typename T>
    void Unsubscribe();
    
    // 處理佇列事件
    virtual void ProcessEvents() = 0;
    virtual void Clear() = 0;
    
    // 統計
    virtual size_t GetHandlerCount() const = 0;
    virtual size_t GetQueuedEventCount() const = 0;
};

// 工廠函式
std::unique_ptr<IEventManager> CreateEventManager();
```

### 事件基類

```cpp
template<typename Derived>
struct Event : public IEvent {
    static std::type_index GetStaticType() {
        return std::type_index(typeid(Derived));
    }
    
    std::type_index GetType() const override {
        return GetStaticType();
    }
};
```

**使用範例**:
```cpp
// 定義事件
struct ButtonClickEvent : public Event<ButtonClickEvent> {
    int buttonId;
    int x, y;
};

// 訂閱事件
eventManager->Subscribe<ButtonClickEvent>([](const ButtonClickEvent& e) {
    std::cout << "Button " << e.buttonId << " clicked at " 
              << e.x << ", " << e.y << std::endl;
});

// 發布事件
eventManager->Publish(ButtonClickEvent{123, 100, 200});
```

---

## 場景管理

### ISceneManager

場景管理器介面。

```cpp
struct ISceneManager {
    virtual ~ISceneManager() = default;
    
    // 初始化
    virtual bool Initialize(IServiceLocator* services) = 0;
    
    // 場景註冊
    virtual void RegisterScene(const std::string& name,
                              SceneFactory factory) = 0;
    
    // 場景控制
    virtual bool SwitchToScene(const std::string& sceneName) = 0;
    virtual void PushScene(const std::string& sceneName) = 0;
    virtual void PopScene() = 0;
    virtual void PopAllScenes() = 0;
    
    // 更新和渲染
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    
    // 輸入處理
    virtual bool HandleInput(const MSG& msg) = 0;
    
    // 查詢
    virtual IScene* GetCurrentScene() const = 0;
    virtual size_t GetSceneStackSize() const = 0;
    virtual bool IsTransitioning() const = 0;
};

// 工廠函式
std::unique_ptr<ISceneManager> CreateSceneManager();
```

### IScene

場景介面。

```cpp
struct IScene {
    virtual ~IScene() = default;
    
    // 生命週期
    virtual bool Initialize(IServiceLocator* services) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Cleanup() = 0;
    
    // 狀態轉換
    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void OnPause() = 0;
    virtual void OnResume() = 0;
    
    // 查詢
    virtual const std::string& GetName() const = 0;
    virtual SceneState GetState() const = 0;
    virtual bool IsTransparent() const = 0;
    
    // 輸入處理
    virtual bool HandleInput(const MSG& msg) = 0;
};
```

---

## 輸入處理

### IInputHandler

輸入處理器介面。

```cpp
struct IInputHandler {
    virtual ~IInputHandler() = default;
    
    // 訊息處理
    virtual HRESULT ProcessMessages() = 0;
    
    // 監聽器管理
    virtual void RegisterListener(IInputListener* listener) = 0;
    virtual void UnregisterListener(IInputListener* listener) = 0;
    
    // 狀態查詢
    virtual bool IsKeyDown(int vkey) const = 0;
    virtual POINT GetMousePosition() const = 0;
    virtual bool IsMouseButtonDown(int button) const = 0;
};

// 工廠函式
std::unique_ptr<IInputHandler> CreateInputHandler(HWND hwnd);
```

### IInputListener

輸入監聽器介面。

```cpp
struct IInputListener {
    virtual ~IInputListener() = default;
    virtual bool HandleMessage(const MSG& msg) = 0;
};
```

---

## 工廠函式

所有主要介面都提供工廠函式來創建實例：

```cpp
// 核心系統
std::unique_ptr<IEngineContext> CreateEngineContext();
std::unique_ptr<ID3DContext> CreateD3DContext();

// 資源管理
std::unique_ptr<IAssetManager> CreateAssetManager();
std::unique_ptr<ITextureManager> CreateTextureManager(ComPtr<IDirect3DDevice9> device);
std::unique_ptr<IEffectManager> CreateEffectManager();
std::unique_ptr<IModelManager> CreateModelManager(
    std::unique_ptr<IModelLoader> loader, ITextureManager* textureManager);

// 渲染系統
std::unique_ptr<IScene3D> CreateScene3D();
std::unique_ptr<ILightManager> CreateLightManager();
std::unique_ptr<IFullScreenQuad> CreateFullScreenQuad();

// UI 系統
std::unique_ptr<IUIManager> CreateUIManager(ITextureManager* textureManager = nullptr);

// 輸入系統
std::unique_ptr<IInputHandler> CreateInputHandler(HWND hwnd);
std::unique_ptr<ICameraController> CreateCameraController(
    IDirect3DDevice9* device, int width, int height);

// 現代架構
std::unique_ptr<IEventManager> CreateEventManager();
std::unique_ptr<IConfigManager> CreateConfigManager();
std::unique_ptr<ISceneManager> CreateSceneManager();
```

## 錯誤處理

大部分函式返回 `HRESULT` 用於錯誤處理：

- `S_OK`: 成功
- `E_FAIL`: 一般失敗
- `E_INVALIDARG`: 無效參數
- `E_OUTOFMEMORY`: 記憶體不足
- `E_POINTER`: 空指標

**範例**:
```cpp
HRESULT hr = engine->Initialize(hwnd, 1280, 720);
if (FAILED(hr)) {
    // 處理錯誤
    MessageBox(hwnd, L"Failed to initialize engine", L"Error", MB_OK);
    return hr;
}
```

## 最佳實踐

1. **使用智慧指標**: 所有工廠函式返回 `std::unique_ptr`
2. **介面導向**: 總是透過介面而非具體類別使用元件
3. **依賴注入**: 使用 `IServiceLocator` 取得依賴
4. **事件驅動**: 使用事件系統進行元件間通訊
5. **RAII**: 確保資源在作用域結束時自動釋放

## 範例程式

```cpp
// 初始化引擎
auto engine = CreateEngineContext();
HRESULT hr = engine->Initialize(hwnd, 1280, 720);

// 取得服務
auto* sceneManager = engine->GetSceneManager();
auto* eventManager = engine->GetEventManager();

// 註冊場景
sceneManager->RegisterScene("MainMenu", CreateMainMenuScene);
sceneManager->RegisterScene("Game", CreateGameScene);

// 設定事件處理
eventManager->Subscribe<ButtonClickEvent>([&](const ButtonClickEvent& e) {
    if (e.buttonId == BUTTON_START_GAME) {
        sceneManager->SwitchToScene("Game");
    }
});

// 開始遊戲
sceneManager->SwitchToScene("MainMenu");
engine->Run();
```