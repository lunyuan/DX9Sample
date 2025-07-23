# 系統架構流程圖

**最後更新**: 2025-07-23  
**版本**: 2.0

## 1. 系統初始化流程

```mermaid
flowchart TD
    A[WinMain Entry] --> B[Create Window]
    B --> C[Create EngineContext]
    C --> D[Initialize DirectX]
    D --> E{Device Type}
    E -->|HAL Hardware| F[Hardware Vertex Processing]
    E -->|HAL Software| G[Software Vertex Processing]
    E -->|REF| H[Reference Rasterizer]
    
    F --> I[Initialize Subsystems]
    G --> I
    H --> I
    
    I --> J[Create TextureManager]
    I --> K[Create EffectManager]
    I --> L[Create ModelManager]
    I --> M[Create LightManager]
    I --> N[Create UIManager]
    I --> O[Create InputHandler]
    I --> P[Create CameraController]
    
    J --> Q[Initialize Modern Systems]
    K --> Q
    L --> Q
    M --> Q
    N --> Q
    O --> Q
    P --> Q
    
    Q --> R[Create EventManager]
    Q --> S[Create ConfigManager]
    Q --> T[Create AssetManager]
    Q --> U[Create ServiceLocator]
    Q --> V[Create SceneManager]
    
    V --> W[Register Scenes]
    W --> X[GameScene]
    W --> Y[PauseScene]
    W --> Z[SettingsScene]
    
    X --> AA[Run Main Loop]
```

## 2. 渲染管線流程

```mermaid
flowchart LR
    A[Main Loop] --> B[Process Input]
    B --> C[Update Systems]
    C --> D[Update Scene]
    D --> E[Clear Buffers]
    E --> F[Begin Scene]
    F --> G[Apply Lights]
    G --> H[Render 3D Objects]
    H --> I[Render UI]
    I --> J[End Scene]
    J --> K[Present]
    K --> A
    
    subgraph "3D Rendering"
        H --> H1[Set View Matrix]
        H1 --> H2[Set Projection Matrix]
        H2 --> H3[Draw Meshes]
        H3 --> H4[Apply Textures]
        H4 --> H5[Apply Effects]
    end
    
    subgraph "UI Rendering"
        I --> I1[Sort by Layer]
        I1 --> I2[Render Background]
        I2 --> I3[Render Components]
        I3 --> I4[Render Text]
    end
```

## 3. 資源載入流程

```mermaid
flowchart TD
    A[LoadAsset Request] --> B{Asset Type}
    B -->|Model| C[IModelLoader]
    B -->|Texture| D[TextureManager]
    B -->|Effect| E[EffectManager]
    
    C --> F{File Format}
    F -->|.x| G[XModelLoader]
    F -->|.fbx| H[FbxLoader]
    F -->|.gltf| I[GltfLoader]
    
    G --> J[Parse File]
    H --> J
    I --> J
    
    J --> K[Create Mesh]
    K --> L[Load Materials]
    L --> M[Load Textures]
    M --> N[Cache Resource]
    N --> O[Return ModelData]
    
    D --> P[Check Cache]
    P -->|Hit| Q[Return Cached]
    P -->|Miss| R[Load from File]
    R --> S[Create D3D Texture]
    S --> T[Add to Cache]
    T --> Q
```

## 4. 事件系統流程

```mermaid
flowchart TD
    A[Event Source] --> B[Publish Event]
    B --> C{Publish Type}
    C -->|Immediate| D[EventManager::Publish]
    C -->|Queued| E[EventManager::QueueEvent]
    
    D --> F[Find Handlers]
    F --> G[Invoke Handlers]
    G --> H[Handler Execution]
    
    E --> I[Add to Queue]
    I --> J[ProcessEvents]
    J --> F
    
    subgraph "Event Subscription"
        K[Component] --> L[EventListener]
        L --> M[Subscribe<EventType>]
        M --> N[Register Handler]
        N --> O[Handler Map]
    end
    
    O --> F
```

## 5. 場景管理流程

```mermaid
flowchart TD
    A[SceneManager] --> B{Scene Operation}
    B -->|Switch| C[Cleanup Current]
    B -->|Push| D[Pause Current]
    B -->|Pop| E[Resume Previous]
    
    C --> F[Scene::OnExit]
    F --> G[Scene::Cleanup]
    G --> H[Load New Scene]
    H --> I[Scene::Initialize]
    I --> J[Scene::OnEnter]
    
    D --> K[Scene::OnPause]
    K --> L[Push to Stack]
    L --> H
    
    E --> M[Pop from Stack]
    M --> N[Scene::OnResume]
    
    subgraph "Scene Lifecycle"
        O[Uninitialized] --> P[Initializing]
        P --> Q[Running]
        Q --> R[Paused]
        R --> Q
        Q --> S[Cleanup]
        S --> O
    end
```

## 6. 輸入處理流程

```mermaid
flowchart TD
    A[Windows Message] --> B[InputHandler::ProcessMessages]
    B --> C{Message Type}
    C -->|Mouse| D[Mouse Events]
    C -->|Keyboard| E[Keyboard Events]
    C -->|System| F[System Events]
    
    D --> G[UIManager Priority]
    G -->|Handled| H[Stop Propagation]
    G -->|Not Handled| I[CameraController]
    I -->|Not Handled| J[Scene Input]
    
    E --> K[Focused Component]
    K -->|Has Focus| L[Component Input]
    K -->|No Focus| M[Global Hotkeys]
    
    F -->|WM_QUIT| N[Exit Application]
    F -->|WM_SIZE| O[Resize Handler]
```

## 7. UI 系統架構

```mermaid
flowchart TD
    A[UIManager] --> B[Root Components]
    B --> C[UIContainer]
    C --> D[Child Components]
    
    D --> E[UIImage]
    D --> F[UIButton]
    D --> G[UIEdit]
    
    subgraph "Component Properties"
        H[Position: Relative/Absolute]
        I[Visibility & Enable State]
        J[Drag Mode]
        K[Layer & Priority]
    end
    
    E --> H
    F --> H
    G --> H
    
    subgraph "Rendering Pipeline"
        L[Calculate Absolute Positions]
        M[Sort by Layer]
        N[Render with Sprite Batch]
        O[Handle Transparency]
    end
    
    C --> L
    L --> M
    M --> N
    N --> O
```

## 8. 依賴注入架構

```mermaid
flowchart TD
    A[EngineContext] --> B[ServiceLocator]
    B --> C[Register Services]
    
    C --> D[AssetManager]
    C --> E[UIManager]
    C --> F[EventManager]
    C --> G[ConfigManager]
    C --> H[SceneManager]
    C --> I[CameraController]
    
    J[Scene] --> K[Initialize with ServiceLocator]
    K --> L[Get Required Services]
    L --> M[Use Services via Interfaces]
    
    subgraph "Service Access"
        N[IAssetManager*]
        O[IUIManager*]
        P[IEventManager*]
        Q[IConfigManager*]
    end
    
    M --> N
    M --> O
    M --> P
    M --> Q
```

## 關鍵架構特點

1. **模組化設計**: 每個子系統都透過介面定義，實現解耦
2. **Factory Pattern**: 所有主要元件都使用工廠函式創建
3. **事件驅動**: 使用類型安全的事件系統進行元件通訊
4. **資源管理**: 智慧指標確保記憶體安全
5. **場景堆疊**: 支援場景覆蓋（如暫停選單）
6. **優先級輸入**: UI 優先處理輸入，然後傳遞給其他系統

## 效能考量

- **紋理快取**: 避免重複載入
- **批次渲染**: UI 使用 sprite batch
- **延遲載入**: 資源按需載入
- **事件佇列**: 避免即時處理阻塞

## 錯誤處理策略

- **HRESULT**: DirectX API 錯誤碼
- **例外處理**: 現代 C++ 例外
- **日誌系統**: OutputDebugString 除錯輸出
- **優雅降級**: 硬體不支援時降級到軟體模式