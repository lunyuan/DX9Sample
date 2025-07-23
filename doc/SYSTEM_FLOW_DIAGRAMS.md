# System Flow Diagrams

## 1. Application Startup Flow

```mermaid
flowchart TD
    Start([WinMain Entry]) --> CreateWindow[Create Win32 Window]
    CreateWindow --> CreateEngine[Create EngineContext]
    CreateEngine --> InitD3D[Initialize Direct3D]
    
    InitD3D --> CreateDevice{Device Created?}
    CreateDevice -->|Yes| InitSubsystems[Initialize Subsystems]
    CreateDevice -->|No| Error1[Show Error & Exit]
    
    InitSubsystems --> CreateManagers[Create Core Managers]
    CreateManagers --> InitServices[Initialize ServiceLocator]
    InitServices --> LoadAssets[Load Initial Assets]
    
    LoadAssets --> CreateScene[Create Initial Scene]
    CreateScene --> MainLoop[Enter Main Loop]
    
    MainLoop --> ProcessMsg{Windows Message?}
    ProcessMsg -->|Yes| HandleMsg[Process Message]
    ProcessMsg -->|No| Update[Update Scene]
    
    HandleMsg --> CheckQuit{Quit Message?}
    CheckQuit -->|Yes| Cleanup[Cleanup & Exit]
    CheckQuit -->|No| MainLoop
    
    Update --> Render[Render Frame]
    Render --> Present[Present to Screen]
    Present --> MainLoop
```

## 2. Model Loading Pipeline

```mermaid
flowchart LR
    subgraph Input
        XFile[.X File]
        FBXFile[.FBX File]
        GLTFFile[.GLTF File]
    end
    
    subgraph AssetManager
        LoadModel[LoadAllModels]
        DetectFormat{Detect Format}
        LoadModel --> DetectFormat
    end
    
    subgraph Loaders
        XLoader[XModelEnhanced]
        FBXLoader[FbxLoader]
        GLTFLoader[GltfLoader]
    end
    
    subgraph Processing
        ParseFile[Parse File]
        ExtractGeo[Extract Geometry]
        ExtractMat[Extract Materials]
        ExtractTex[Extract Textures]
        CreateData[Create ModelData]
    end
    
    subgraph TextureManager
        CheckCache{In Cache?}
        LoadTexture[Load from Disk]
        AddCache[Add to Cache]
    end
    
    subgraph Output
        ModelData[ModelData Structure]
        Scene[Add to Scene]
    end
    
    XFile --> LoadModel
    FBXFile --> LoadModel
    GLTFFile --> LoadModel
    
    DetectFormat -->|.x| XLoader
    DetectFormat -->|.fbx| FBXLoader
    DetectFormat -->|.gltf/.glb| GLTFLoader
    
    XLoader --> ParseFile
    FBXLoader --> ParseFile
    GLTFLoader --> ParseFile
    
    ParseFile --> ExtractGeo
    ExtractGeo --> ExtractMat
    ExtractMat --> ExtractTex
    
    ExtractTex --> CheckCache
    CheckCache -->|No| LoadTexture
    CheckCache -->|Yes| AddCache
    LoadTexture --> AddCache
    
    ExtractGeo --> CreateData
    ExtractMat --> CreateData
    AddCache --> CreateData
    
    CreateData --> ModelData
    ModelData --> Scene
```

## 3. Rendering Pipeline

```mermaid
flowchart TD
    subgraph Frame Start
        BeginFrame[Begin Frame]
        Clear[Clear Buffers]
    end
    
    subgraph Scene Processing
        UpdateScene[Update Scene]
        UpdateCamera[Update Camera]
        UpdateLights[Update Lights]
        CullObjects{Frustum Culling}
    end
    
    subgraph Render Queue
        SortObjects[Sort by Material/Distance]
        SetupStates[Setup Render States]
    end
    
    subgraph Object Rendering
        ForEach[For Each Visible Object]
        SetMaterial[Set Material]
        SetTexture[Set Texture]
        SetShader[Set Shader]
        DrawMesh[Draw Mesh]
    end
    
    subgraph UI Rendering
        RenderUI[Render UI Layer]
        RenderText[Render Text]
        RenderButtons[Render Buttons]
    end
    
    subgraph Frame End
        Present[Present Frame]
        EndFrame[End Frame]
    end
    
    BeginFrame --> Clear
    Clear --> UpdateScene
    UpdateScene --> UpdateCamera
    UpdateCamera --> UpdateLights
    UpdateLights --> CullObjects
    
    CullObjects -->|Visible| SortObjects
    CullObjects -->|Culled| Skip[Skip Object]
    
    SortObjects --> SetupStates
    SetupStates --> ForEach
    
    ForEach --> SetMaterial
    SetMaterial --> SetTexture
    SetTexture --> SetShader
    SetShader --> DrawMesh
    DrawMesh -->|More Objects| ForEach
    DrawMesh -->|Done| RenderUI
    
    RenderUI --> RenderText
    RenderText --> RenderButtons
    RenderButtons --> Present
    Present --> EndFrame
```

## 4. Texture Loading Flow (FBX Specific)

```mermaid
flowchart TD
    FBXFile[FBX File] --> ExtractMat[ExtractMaterials]
    
    ExtractMat --> FindDiffuse[Find DiffuseColor Property]
    FindDiffuse --> GetSrcObj[Get Source Objects]
    
    GetSrcObj --> CheckType{Is FbxFileTexture?}
    CheckType -->|Yes| GetFileName[Get Texture Filename]
    CheckType -->|No| NoTexture[No Texture]
    
    GetFileName --> Method1[Try GetRelativeFileName]
    Method1 --> CheckValid1{Valid Path?}
    CheckValid1 -->|No| Method2[Try GetFileName]
    CheckValid1 -->|Yes| LoadFile
    
    Method2 --> CheckValid2{Valid Path?}
    CheckValid2 -->|No| ListProps[List All Properties]
    CheckValid2 -->|Yes| LoadFile
    
    subgraph Path Resolution
        LoadFile[LoadTextureFromFile]
        TryAbs{Absolute Path?}
        TryRel[Try Relative to FBX]
        TryTest[Try test/ Directory]
        TryCurrent[Try Current Dir]
    end
    
    LoadFile --> TryAbs
    TryAbs -->|Yes| CheckExists1{File Exists?}
    TryAbs -->|No| TryRel
    
    CheckExists1 -->|Yes| CreateTex[D3DXCreateTextureFromFile]
    CheckExists1 -->|No| TryRel
    
    TryRel --> CheckExists2{File Exists?}
    CheckExists2 -->|Yes| CreateTex
    CheckExists2 -->|No| TryTest
    
    TryTest --> CheckExists3{File Exists?}
    CheckExists3 -->|Yes| CreateTex
    CheckExists3 -->|No| TryCurrent
    
    TryCurrent --> CheckExists4{File Exists?}
    CheckExists4 -->|Yes| CreateTex
    CheckExists4 -->|No| Failed[Texture Load Failed]
    
    CreateTex --> Success[Material.tex = texture]
```

## 5. Event System Flow

```mermaid
flowchart LR
    subgraph Publisher
        Component[Component] --> PublishEvent[Publish Event]
    end
    
    subgraph EventManager
        PublishEvent --> CheckImmediate{Immediate?}
        CheckImmediate -->|Yes| ProcessNow[Process Immediately]
        CheckImmediate -->|No| QueueEvent[Add to Queue]
        
        QueueEvent --> EventQueue[(Event Queue)]
        EventQueue --> ProcessQueue[Process Queue]
    end
    
    subgraph Subscribers
        Listener1[UI Manager]
        Listener2[Scene Manager]
        Listener3[Audio System]
        Listener4[Game Logic]
    end
    
    ProcessNow --> Notify[Notify Subscribers]
    ProcessQueue --> Notify
    
    Notify --> Listener1
    Notify --> Listener2
    Notify --> Listener3
    Notify --> Listener4
    
    Listener1 --> HandleEvent1[Handle UI Event]
    Listener2 --> HandleEvent2[Handle Scene Event]
    Listener3 --> HandleEvent3[Handle Audio Event]
    Listener4 --> HandleEvent4[Handle Game Event]
```

## 6. Scene Management Flow

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    
    Uninitialized --> Initializing: Initialize()
    Initializing --> Initialized: Success
    Initializing --> Error: Failure
    
    Initialized --> Entering: OnEnter()
    Entering --> Active: Complete
    
    Active --> Updating: Update()
    Updating --> Active: Continue
    
    Active --> Rendering: Render()
    Rendering --> Active: Continue
    
    Active --> Pausing: Pause Event
    Pausing --> Paused: OnPause()
    
    Paused --> Resuming: Resume Event
    Resuming --> Active: OnResume()
    
    Active --> Exiting: Scene Change
    Paused --> Exiting: Scene Change
    
    Exiting --> Cleanup: OnExit()
    Cleanup --> [*]: Complete
    
    Error --> [*]: Abort
```

## 7. Resource Management Flow

```mermaid
flowchart TD
    subgraph Request
        LoadReq[Load Request] --> CheckCache{In Cache?}
    end
    
    subgraph Cache Management
        Cache[(Resource Cache)]
        RefCount[Reference Count]
    end
    
    subgraph Loading
        LoadDisk[Load from Disk]
        Validate[Validate Resource]
        Process[Process Resource]
    end
    
    subgraph Memory Management
        CheckMem{Memory Available?}
        Evict[Evict LRU Resources]
        Allocate[Allocate Memory]
    end
    
    CheckCache -->|Yes| IncRef[Increment RefCount]
    CheckCache -->|No| CheckMem
    
    CheckMem -->|Yes| LoadDisk
    CheckMem -->|No| Evict
    
    Evict --> CheckMem
    
    LoadDisk --> Validate
    Validate -->|Valid| Process
    Validate -->|Invalid| Error[Return Error]
    
    Process --> Allocate
    Allocate --> AddCache[Add to Cache]
    AddCache --> SetRef[Set RefCount = 1]
    
    IncRef --> ReturnRes[Return Resource]
    SetRef --> ReturnRes
    
    ReturnRes --> UseResource[Use Resource]
    UseResource --> Release{Release?}
    
    Release -->|Yes| DecRef[Decrement RefCount]
    Release -->|No| UseResource
    
    DecRef --> CheckZero{RefCount == 0?}
    CheckZero -->|Yes| MarkEvict[Mark for Eviction]
    CheckZero -->|No| KeepCache[Keep in Cache]
```

## 8. Input Processing Flow

```mermaid
flowchart TD
    subgraph Windows Messages
        WndProc[Window Procedure]
        MouseMsg[Mouse Messages]
        KeyMsg[Keyboard Messages]
    end
    
    subgraph Input Handler
        ProcessInput[Process Input]
        UpdateState[Update Input State]
        CheckListeners{Has Listeners?}
    end
    
    subgraph Input Consumers
        Camera[Camera Controller]
        UI[UI Manager]
        Game[Game Logic]
    end
    
    WndProc --> MouseMsg
    WndProc --> KeyMsg
    
    MouseMsg --> ProcessInput
    KeyMsg --> ProcessInput
    
    ProcessInput --> UpdateState
    UpdateState --> CheckListeners
    
    CheckListeners -->|Yes| NotifyAll[Notify All Listeners]
    CheckListeners -->|No| Store[Store State]
    
    NotifyAll --> Priority{Check Priority}
    
    Priority -->|UI First| UI
    UI --> Consumed1{Consumed?}
    Consumed1 -->|No| Camera
    Consumed1 -->|Yes| Done1[End Processing]
    
    Camera --> Consumed2{Consumed?}
    Consumed2 -->|No| Game
    Consumed2 -->|Yes| Done2[End Processing]
    
    Game --> Done3[End Processing]
```

## Usage Notes

These diagrams illustrate the key flows within the DX9Sample engine:

1. **Startup Flow**: Shows initialization sequence and main loop
2. **Model Loading**: Details the asset loading pipeline
3. **Rendering**: Describes frame rendering process
4. **Texture Loading**: FBX-specific texture resolution
5. **Event System**: Event publishing and handling
6. **Scene Management**: Scene lifecycle states
7. **Resource Management**: Cache and memory handling
8. **Input Processing**: Input event routing

Each diagram can be rendered using Mermaid-compatible tools or documentation systems that support Mermaid syntax.