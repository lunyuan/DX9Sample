# Current Architecture Overview

## 1. Engine Architecture

### Core Pattern
The engine follows a modular interface-based architecture:
```
EngineContext (orchestrator) → Subsystem Interfaces → Concrete Implementations
```

### Key Design Principles
- **Interface-Driven Design**: All major components have interface definitions (`I*.h` files)
- **Factory Pattern**: Each interface provides factory functions for dependency injection
- **RAII Memory Management**: Using smart pointers throughout (no manual memory management)
- **Service Locator Pattern**: Centralized access to engine services

## 2. Core Systems

### Graphics Systems
- **ID3DContext** → `D3DContext`: DirectX device management
- **IModelManager** → `ModelManager`: 3D model loading/management
- **ITextureManager** → `TextureManager`: Thread-safe texture caching
- **IEffectManager** → `EffectManager`: Shader effects (.fx files)
- **ILightManager** → `LightManager`: Lighting system
- **IScene3D** → `Scene3D`: Scene rendering pipeline

### Input/Camera
- **ICameraController** → `CameraController`: Camera movement and controls
- **IInputHandler** → `InputHandler`: Input processing

### Modern Architecture Systems
- **IAssetManager** → `AssetManager`: Unified asset loading with path resolution
- **IConfigManager** → `JsonConfigManager`: JSON-based configuration
- **ISceneManager** → `SceneManager`: Scene stack management and transitions
- **IUIManager** → `UIManager`: Advanced UI system with drag-drop support
- **IEventManager** → `EventManager`: Type-safe event system
- **IServiceLocator** → `ServiceLocator`: Dependency injection container

## 3. Model Loading System

### Supported Formats
- **DirectX .x files**: Primary format (XFileLoader/XModelLoader)
- **Autodesk FBX**: Industry standard format (FbxLoader)
- **glTF 2.0**: Modern web-friendly format (GltfLoader)

### Animation System
- Skeletal animation with `Skeleton` and `AnimationPlayer` classes
- Vertex skinning via HLSL shaders
- Support for bone hierarchies and animation blending

## 4. UI System Architecture

### Component Hierarchy
```
UIManager
├── Legacy System (for compatibility)
│   ├── UITextElement
│   ├── UIImageElement
│   └── UIButton
└── New Component System
    ├── UIComponentNew (base class)
    ├── UIImageNew
    ├── UIButtonNew
    └── UIEditNew
```

### UI Features
- **Parent-Child Relationships**: Components can have children that move together
- **Layer-Based Rendering**: Multiple layers with priority support
- **Transparency Support**: Pixel-perfect hit testing for images
- **Dynamic Text**: ID-based text update system
- **Drag-Drop System**: Complete drag-and-drop functionality

## 5. Scene Management

### Scene Stack
- Push/pop scene operations
- Transparent scenes for overlays (pause menus, etc.)
- Scene lifecycle: Initialize → OnEnter → Update/Render → OnExit → Cleanup

### Current Scenes
- **GameScene**: Main gameplay scene with 3D rendering and UI
- **PauseScene**: Overlay scene for pause menu
- **SettingsScene**: Configuration and settings

## 6. Event System

### Type-Safe Events
- Compile-time type checking
- Immediate and queued event processing
- EventListener base class for automatic subscription management

### Common Events
- `UIComponentClicked`
- `SceneChanged`
- `AssetLoaded`
- `ConfigurationChanged`
- `GameStateChanged`

## 7. Memory Management

### Smart Pointer Usage
- `std::unique_ptr`: For interface implementations
- `std::shared_ptr`: For shared resources (textures, models)
- `Microsoft::WRL::ComPtr`: For COM objects (DirectX interfaces)
- Custom `UniqueWithWeak`: For objects needing weak references

## 8. Build System

### Dependencies
- DirectX SDK (June 2010)
- FBX SDK 2020.3.7
- Windows 10 SDK
- Visual Studio 2022 (v143 toolset)
- C++20 standard

### Project Structure
```
DX9Sample/
├── Src/          # Source files
├── test/         # Runtime directory with assets
├── doc/          # Documentation
├── shaders/      # HLSL shader files
└── DX9Sample.sln # Visual Studio solution
```

## 9. Asset Pipeline

### Asset Loading Flow
1. AssetManager receives load request
2. Resolves path based on asset type
3. Checks cache for existing asset
4. Loads using appropriate loader
5. Stores in cache with reference counting
6. Returns shared pointer to asset

### Hot-Reload Support
- File watcher for development
- Automatic asset reloading
- Cache invalidation

## 10. Rendering Pipeline

### Frame Flow
1. EngineContext updates all systems
2. SceneManager updates active scenes
3. Scene3D renders 3D content
4. UIManager renders UI overlay
5. Present to screen

### Shader System
- Fixed function pipeline for basic rendering
- Programmable pipeline for advanced effects
- Skeletal animation vertex shader
- Post-processing support