# DirectX 9 引擎架構詳細分析

## 1. 系統初始化流程圖

```mermaid
flowchart TD
    A[WinMain 啟動] --> B[視窗類別註冊 WNDCLASSEX]
    B --> C[建立視窗 CreateWindowEx]
    C --> D[顯示視窗 ShowWindow]
    D --> E[建立 EngineContext]
    E --> F[EngineContext::Initialize]
    
    F --> G[參數驗證]
    G --> H{驗證成功?}
    H -->|否| I[返回 E_INVALIDARG]
    H -->|是| J[建立子系統]
    
    J --> K[建立 TextureManager]
    K --> L[建立 EffectManager]
    L --> M[建立 D3DContext]
    M --> N[建立 ModelManager]
    N --> O[建立 LightManager]
    O --> P[建立 Scene3D]
    P --> Q[建立 UIManager]
    Q --> R[建立 InputHandler]
    R --> S[建立 CameraController]
    S --> T[建立 FullScreenQuad]
    
    T --> U{所有子系統建立成功?}
    U -->|否| V[返回 E_FAIL]
    U -->|是| W[執行主迴圈 Run()]
    
    W --> X[訊息處理迴圈]
    X --> Y[PeekMessage]
    Y --> Z{有訊息?}
    Z -->|是| AA[處理訊息]
    Z -->|否| BB[更新邏輯]
    AA --> BB
    BB --> CC[輸入處理]
    CC --> DD[相機更新]
    DD --> EE[開始渲染]
    EE --> FF[應用效果]
    FF --> GG[應用光照]
    GG --> HH[場景渲染]
    HH --> II[UI渲染]
    II --> JJ[後處理]
    JJ --> KK[Present 到螢幕]
    KK --> LL{程式結束?}
    LL -->|否| Y
    LL -->|是| MM[清理資源]
    MM --> NN[程式結束]
```

## 2. 核心架構組件圖

```mermaid
classDiagram
    class EngineContext {
        -device_: ComPtr~IDirect3DDevice9~
        -hwnd_: HWND
        -width_: UINT
        -height_: UINT
        +Initialize()
        +Run()
        +Get各種Manager()
    }
    
    class ITextureManager {
        <<interface>>
        +Load(filepath)
        +Get(key)
        +Clear()
    }
    
    class TextureManager {
        -device_: ComPtr~IDirect3DDevice9~
        -cache_: unordered_map
        -mutex_: shared_mutex
        +Load(filepath)
        +Get(key)
        +Clear()
    }
    
    class IModelManager {
        <<interface>>
        +LoadModels()
        +GetModel()
        +Clear()
    }
    
    class ModelManager {
        -loader_: unique_ptr~IModelLoader~
        -models_: map~string, ModelData~
        +LoadModels()
        +GetModel()
        +Clear()
    }
    
    class IScene3D {
        <<interface>>
        +Init()
        +Render()
    }
    
    class Scene3D {
        -mesh_: ComPtr~ID3DXMesh~
        -tex_: ComPtr~IDirect3DTexture9~
        -fx_: ComPtr~ID3DXEffect~
        -lightMgr_: ILightManager*
        +Init()
        +Render()
    }
    
    class ICameraController {
        <<interface>>
        +Update()
        +GetViewMatrix()
        +HandleMessage()
    }
    
    class CameraController {
        -position_: XMFLOAT3
        -target_: XMFLOAT3
        -up_: XMFLOAT3
        +Update()
        +GetViewMatrix()
        +HandleMessage()
    }
    
    EngineContext --> ITextureManager
    EngineContext --> IModelManager
    EngineContext --> IScene3D
    EngineContext --> ICameraController
    ITextureManager <|-- TextureManager
    IModelManager <|-- ModelManager
    IScene3D <|-- Scene3D
    ICameraController <|-- CameraController
```

## 3. 渲染管線流程圖

```mermaid
flowchart LR
    A[開始渲染 BeginScene] --> B[應用效果 EffectManager]
    B --> C[設定光照 LightManager]
    C --> D[場景渲染 Scene3D]
    
    D --> E[設定相機矩陣]
    E --> F[載入材質貼圖]
    F --> G[設定 Shader 參數]
    G --> H[執行 Effect Technique]
    H --> I[繪製網格 DrawSubset]
    
    I --> J[UI 渲染 UIManager]
    J --> K[後處理 FullScreenQuad]
    K --> L[Present 到螢幕]
    
    subgraph "Scene3D 詳細流程"
        E --> E1[設定 World/View/Projection 矩陣]
        F --> F1[從 TextureManager 取得貼圖]
        G --> G1[設定光照方向]
        G1 --> G2[設定 WVP 矩陣]
        H --> H1[Begin Effect]
        H1 --> H2[BeginPass]
        I --> I1[mesh_->DrawSubset]
        I1 --> I2[EndPass]
        I2 --> I3[End Effect]
    end
```

## 4. 模型載入工作流程

```mermaid
flowchart TD
    A[模型載入請求] --> B[ModelManager::LoadModels]
    B --> C[選擇 Loader 類型]
    
    C --> D{檔案格式?}
    D -->|.x| E[XModelLoader]
    D -->|.fbx| F[FbxLoader]
    D -->|.gltf| G[GltfLoader]
    
    E --> H[解析 X 檔案格式]
    F --> I[解析 FBX 格式]
    G --> J[解析 glTF 格式]
    
    H --> K[建立 SkinMesh]
    I --> K
    J --> K
    
    K --> L[解析材質資訊]
    L --> M[解析骨架結構]
    M --> N[解析動畫資料]
    N --> O[建立 Vertex/Index Buffers]
    O --> P[儲存到 ModelData]
    P --> Q[加入 models_ 快取]
    Q --> R[載入完成]
    
    subgraph "SkinMesh 結構"
        K --> K1[vertices_: vector~SkinnedVertex~]
        K --> K2[indices_: vector~DWORD~]
        K --> K3[materials_: vector~Material~]
        K --> K4[texture_paths_: vector~string~]
    end
```

## 5. 資源管理系統圖

```mermaid
flowchart TD
    A[資源請求] --> B{資源類型?}
    
    B -->|貼圖| C[TextureManager]
    B -->|模型| D[ModelManager]
    B -->|效果| E[EffectManager]
    
    C --> F[檢查快取 cache_]
    F --> G{快取中存在?}
    G -->|是| H[返回 shared_ptr]
    G -->|否| I[從檔案載入]
    I --> J[D3DXCreateTextureFromFile]
    J --> K[加入快取]
    K --> H
    
    D --> L[檢查 models_ map]
    L --> M{模型已載入?}
    M -->|是| N[返回 ModelData*]
    M -->|否| O[透過 IModelLoader 載入]
    O --> P[儲存到 models_]
    P --> N
    
    E --> Q[效果檔案處理]
    Q --> R[D3DXCreateEffectFromFile]
    R --> S[設定 Shader 參數]
    
    subgraph "執行緒安全機制"
        C --> C1[shared_mutex 保護]
        F --> C1
        I --> C1
        K --> C1
    end
    
    subgraph "記憶體管理"
        H --> H1[shared_ptr 自動釋放]
        N --> N1[ModelData 生命週期管理]
        S --> S1[ComPtr 管理 D3DX 物件]
    end
```

## 6. 訊息處理與輸入系統

```mermaid
flowchart LR
    A[Windows 訊息] --> B[WndProc]
    B --> C[建立 MSG 結構]
    C --> D[CameraController::HandleMessage]
    D --> E{相機處理成功?}
    E -->|是| F[返回 0]
    E -->|否| G[UIManager::HandleMessage]
    G --> H{UI 處理成功?}
    H -->|是| F
    H -->|否| I[DefWindowProc]
    
    subgraph "InputHandler 系統"
        J[InputHandler::PollMessage] --> K[取得鍵盤狀態]
        K --> L[通知 IInputListener]
        L --> M[CameraController 更新]
    end
    
    subgraph "相機控制邏輯"
        M --> M1[處理滑鼠移動]
        M1 --> M2[處理鍵盤輸入]
        M2 --> M3[更新 position/target/up]
        M3 --> M4[計算 View Matrix]
    end
```

## 7. 效果系統架構

```mermaid
graph TD
    A[EffectManager] --> B[載入 .fx 檔案]
    B --> C[D3DXCreateEffectFromFile]
    C --> D[快取 Effect 物件]
    
    D --> E[設定 Technique]
    E --> F[設定參數]
    F --> G[Begin/EndPass 執行]
    
    subgraph "Shader 參數設定"
        F --> F1[矩陣參數 g_WVP, g_View, g_Proj]
        F --> F2[光照參數 g_LightDir]
        F --> F3[材質貼圖 g_DiffuseTex]
        F --> F4[其他自定義參數]
    end
    
    subgraph "Effect 生命週期"
        G --> G1[fx_->Begin]
        G1 --> G2[fx_->BeginPass]
        G2 --> G3[繪製幾何體]
        G3 --> G4[fx_->EndPass]
        G4 --> G5[fx_->End]
    end
```

## 8. 系統依賴關係圖

```mermaid
graph TB
    A[main.cpp] --> B[EngineContext]
    B --> C[D3DContext - DirectX 裝置管理]
    B --> D[TextureManager - 材質快取]
    B --> E[ModelManager - 模型管理]
    B --> F[EffectManager - Shader 效果]
    B --> G[LightManager - 光照系統]
    B --> H[Scene3D - 場景渲染]
    B --> I[UIManager - 使用者介面]
    B --> J[InputHandler - 輸入處理]
    B --> K[CameraController - 相機控制]
    B --> L[FullScreenQuad - 後處理]
    
    E --> M[IModelLoader 介面]
    M --> N[XModelLoader - X檔案載入]
    M --> O[FbxLoader - FBX載入]
    M --> P[GltfLoader - glTF載入]
    
    E --> Q[ModelData - 模型資料容器]
    Q --> R[SkinMesh - 蒙皮網格]
    Q --> S[Skeleton - 骨架系統]
    
    H --> D
    H --> F
    H --> G
    
    J --> K
    
    subgraph "外部相依"
        T[DirectX 9 SDK]
        U[FBX SDK]
        V[DirectXMath]
        W[WRL ComPtr]
    end
    
    C --> T
    N --> T
    O --> U
    B --> V
    B --> W
```

這個詳細的架構分析展示了整個 DirectX 9 引擎的系統化流程，從初始化到渲染，再到資源管理的完整工作流程。每個組件都有明確的職責分工，並透過介面實現了良好的模組化設計。