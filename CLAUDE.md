# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

**Build Command (MSBuild):** 
```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.sln -p:Configuration=Debug -p:Platform=x64
```
**Alternative:** Open `DX9Sample.sln` in Visual Studio 2022 and build (F7)
- Project uses Visual Studio 2022 (v143 toolset) with C++20
- Supports Win32 and x64 platforms (Debug/Release)
- Output executable goes to `test/` directory

**Dependencies Required:**
- DirectX SDK (June 2010)
- FBX SDK 2020.3.7
- Windows 10 SDK

**Runtime:** Executable runs from `test/` directory where assets are located

## Architecture Overview

This is a **modular DirectX 9 3D graphics engine** with interface-based architecture. The core pattern is:

```
EngineContext (orchestrator) � Subsystem Interfaces � Concrete Implementations
```

### Core Interfaces and Implementations

All major components follow interface/implementation pattern:

**Core Engine Systems:**
- `ID3DContext` � `D3DContext` - DirectX device management
- `IModelManager` � `ModelManager` - 3D model loading/management  
- `ITextureManager` � `TextureManager` - Texture caching with thread safety
- `IEffectManager` � `EffectManager` - Shader effects (.fx files)
- `ILightManager` � `LightManager` - Lighting system
- `IScene3D` � `Scene3D` - Scene rendering pipeline
- `ICameraController` � `CameraController` - Camera movement
- `IInputHandler` � `InputHandler` - Input processing

**Modern Architecture Systems:**
- `IAssetManager` � `AssetManager` - Unified asset loading with path resolution
- `IConfigManager` � `JsonConfigManager` - JSON-based configuration system
- `ISceneManager` � `SceneManager` - Scene stack management and transitions
- `IUIManager` → `UIManager` - UI management with parent-child relationships and dynamic text updates
- `IEventManager` � `EventManager` - Type-safe event system for component communication
- `IServiceLocator` � `ServiceLocator` - Dependency injection container

### Model Loading System

The engine supports multiple 3D formats through dedicated loaders:

- **XFileLoader/XModelLoader**: DirectX .x files (basic single-mesh support)
- **XModelEnhanced**: Enhanced X file loader with multi-object separation
- **FbxLoader**: Autodesk FBX format
- **GltfLoader**: glTF 2.0 format
- **SkinMesh**: Unified skinned mesh container with animation support

Animation system uses skeletal animation with `Skeleton` and `AnimationPlayer` classes.

**X File Loading Notes:**
- Use `LoadAllModels("test.x")` to load all objects from an X file
- Default camera distance: 50.0f (adjustable with mouse controls)
- Model scale: 10.0f for proper visibility
- Supports multi-mesh X files with proper separation
- Textures are loaded and preserved correctly

**FBX File Loading Notes:**
- FBX texture loading is now fully implemented (2025-07-23)
- Textures are loaded from relative paths or test/ directory
- Use `LoadAllModels("model.fbx")` to load with textures
- Supports conversion from X to FBX with texture preservation
- See `doc/FBX_TEXTURE_LOADING_IMPLEMENTATION.md` for technical details

### Key Patterns

**Factory Pattern Implementation:**
- All interfaces provide Factory functions: `std::unique_ptr<IInterface> CreateInterface(...)`
- Factory functions enable dependency injection and interface-based design
- EngineContext uses Factory functions instead of direct concrete class instantiation
- Examples: `CreateD3DContext()`, `CreateUIManager()`, `CreateModelManager()`, etc.

**Resource Management:** 
- `std::unique_ptr` for custom interface classes (IEffectManager, ITextureManager, etc.)
- `UniqueWithWeak` for objects needing weak reference capability
- `Microsoft::WRL::ComPtr` for COM objects (IDirect3DDevice9, ID3DXEffect, etc.)
- `std::shared_ptr` for shared resources (textures in cache)
- Thread-safe texture manager with `std::shared_mutex`

**Memory Management:** RAII throughout, smart pointers, no manual memory management

**Interface Design:**
- All public methods in concrete classes have corresponding virtual declarations in interfaces
- Concrete classes use `override` keyword for all interface implementations
- Strict interface usage - classes only use other components through interfaces, not concrete types

**Entry Point:** `main.cpp` creates EngineContext, loads assets from test/ directory, runs render loop

### Modern Game Engine Architecture

The engine has been redesigned to follow modern game engine patterns inspired by Unity and Unreal Engine:

**Service Locator Pattern:**
- `ServiceLocator` provides centralized access to all engine services
- Scenes receive services through dependency injection
- Clean separation of concerns between systems

**Scene Management:**
- `ISceneManager` supports scene stack with push/pop operations
- Transparent scenes for overlays (pause menus, inventories)
- Scene lifecycle: Initialize → OnEnter → Update/Render → OnExit → Cleanup

**UI Management System:**
- `IUIManager` provides comprehensive UI element management
- Parent-child relationships for grouped UI elements
- Dynamic text updates with ID-based system (AddText returns ID, UpdateText modifies content)
- Layer-based rendering with priority support
- Support for buttons, images, text, and edit controls
- Mouse interaction with drag support for parent containers

**Type-Safe Event System:**
- `IEventManager` provides compile-time type checking for events
- Support for immediate publishing and queued processing
- `EventListener` base class for automatic subscription management
- Decoupled communication between engine systems

**Asset Management:**
- `IAssetManager` with configurable path resolution
- Thread-safe caching with reference counting
- Support for multiple asset types with unified interface
- Hot-reload capability for development

## File Organization

- `Src/I*.h` - Interface definitions
- `Src/*Manager.*` - Subsystem implementations  
- `Src/*Loader.*` - Model format loaders
- `Src/EngineContext.*` - Core engine orchestrator
- `test/` - Assets, textures, and test models (working directory)
- `doc/` - Generated documentation files

## Generated Documentation

The `doc/` directory contains comprehensive documentation organized by category. See `doc/README.md` for a complete index.

### Key Documentation:
- **Architecture**: `ARCHITECTURE_ANALYSIS.md`, `CURRENT_ARCHITECTURE.md`, `FACTORY_FUNCTIONS_AUDIT.md`, `SYSTEM_ARCHITECTURE_ANALYSIS.md`, `ARCHITECTURE_CONSOLIDATION_PLAN.md`
- **Build & Setup**: `BUILD_INSTRUCTIONS.md`, `TESTING_INSTRUCTIONS.md`
- **System Guides**: `EVENT_SYSTEM_USAGE.md`, `SCENE_MANAGER_USAGE.md`, `ASSET_MANAGER_USAGE.md`, `UI_SYSTEM_USAGE.md`
- **Model System**: `MODEL_LOADER_ANALYSIS.md`, `MODEL_LOADING_EXAMPLES.md`, `X_FILE_MULTI_OBJECT_LOADING.md`, `FBX_TEXTURE_LOADING_IMPLEMENTATION.md`
- **Implementation Details**: `CAMERA_FIX.md`, `TEXTURE_MANAGER_ARCHITECTURE.md`, `UI_DRAGGING_STATUS.md`
- **Planning & Optimization**: `TODO_AND_ROADMAP.md`, `SYSTEM_OPTIMIZATION_ROADMAP.md`, `UNUSED_COMPONENTS_CLEANUP_LIST.md`
- **System Analysis**: `SYSTEM_FLOW_DIAGRAMS.md`

These documents provide detailed insights into the codebase structure, implementation decisions, and development process.

## Development Notes

- All major systems use dependency injection through constructors
- Error handling via HRESULT return codes
- Modern C++20 features used throughout (filesystem, smart pointers)
- Assets loaded from relative paths in test/ directory

## Best Practices

**Before Major Refactoring:**
- Create documentation in `doc/` folder describing the refactoring plan
- Analyze current state and dependencies
- List files to be modified/removed
- Document risks and considerations
- This helps prevent forgetting important details during complex changes

Example: See `doc/MODEL_LOADER_REFACTORING.md` for model loader consolidation

## Current Architecture Status (2025-07-23)

**Dual Architecture in Transition:**
The engine currently maintains both legacy and modern architectures for backward compatibility:
- **Legacy**: Direct component access via getter methods
- **Modern**: ServiceLocator pattern with dependency injection

See `doc/ARCHITECTURE_CONSOLIDATION_PLAN.md` for the migration strategy.

**Recent Improvements:**
- FBX texture loading fully implemented
- Multi-object X file support enhanced
- Documentation comprehensively updated

**Cleanup Status (2025-07-23):**
Phase 1 cleanup completed - removed ~3,000 lines of unused code:
- ✅ ImprovedTextureManager (deleted)
- ✅ RenderTargetManager (deleted)
- ✅ ModelExtractor (deleted)
- ✅ Test scenes (deleted)
- ✅ V2 model system files (deleted)
- ✅ d3dfile utilities (deleted)

**Optimization Roadmap:**
See `doc/SYSTEM_OPTIMIZATION_ROADMAP.md` for performance improvement plan