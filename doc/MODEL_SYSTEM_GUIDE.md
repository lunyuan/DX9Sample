# Model System Guide

## Overview

The DX9Sample engine features a comprehensive model loading system that supports multiple 3D file formats through a unified interface-based architecture. This guide covers the model loading architecture, implementation details, usage examples, and system optimization strategies.

## Architecture Overview

### Core Components

#### 1. IModelLoader Interface
The foundation of the model loading system, providing a unified interface for all format loaders:

```cpp
struct IModelLoader {
  virtual ~IModelLoader() = default;
  
  // Load all models from file
  [[nodiscard]] virtual std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const = 0;
  
  // Get model names without loading
  [[nodiscard]] virtual std::vector<std::string>
    GetModelNames(const std::filesystem::path& file) const = 0;
};
```

#### 2. Model Loaders
- **XModelEnhanced/XModelEnhancedLoader**: Enhanced X file loader with multi-object separation
- **FbxLoader**: Autodesk FBX format support with full material/texture loading
- **GltfModelLoader**: glTF 2.0 format support with PBR material handling
- **XModelLoader**: Legacy X file loader (partially active)

#### 3. ModelData Structure
```cpp
struct ModelData {
  SkinMesh mesh;              // Mesh data with materials
  Skeleton skeleton;          // Skeletal hierarchy
  std::vector<std::string> animationNames;  // Animation references
  bool useOriginalTextures = true;          // Texture loading control
};
```

### Integration with Engine Systems

#### AssetManager Integration
The AssetManager automatically selects the appropriate loader based on file extension:

```cpp
// AssetManager.cpp - Loader Selection
if (extension == ".x") {
    auto enhancedModels = XModelEnhanced::LoadWithSeparation(filePath, device_);
} else if (extension == ".fbx") {
    models = fbxLoader_->Load(filePath, device_);
} else if (extension == ".gltf" || extension == ".glb") {
    GltfModelLoader gltfLoader;
    models = gltfLoader.Load(filePath, device_);
}
```

## Loading Examples

### Basic Model Loading

#### Load All Models from a File
```cpp
// Using AssetManager (recommended)
auto models = assetManager->LoadAllModels("test/horse_group.x");

// Direct loader usage
XModelEnhancedLoader loader;
auto models = loader.Load(L"test/horse_group.x", device.Get());
```

#### Query Available Models Without Loading
```cpp
// Check what models are in a file
auto availableModels = modelManager->GetAvailableModels(L"test/horse_group.x");
for (const auto& name : availableModels) {
    std::cout << "Available model: " << name << std::endl;
}
```

#### Load Specific Model
```cpp
// Load only a specific model from a multi-model file
bool success = modelManager->LoadModel(L"test/horse_group.x", "Horse1", device.Get());
```

#### Load and Rename Model
```cpp
// Load model with a custom name
bool success = modelManager->LoadModelAs(
    L"test/horse_group.x", 
    "Horse1",           // Original name in file
    "PlayerHorse",      // New name in manager
    device.Get()
);
```

### Advanced Usage Scenarios

#### Dynamic Character Loading
```cpp
void LoadCharacter(const std::string& characterType) {
    std::wstring fileName = L"models/" + std::wstring(characterType.begin(), characterType.end()) + L".x";
    
    // Query available parts
    auto models = modelManager->GetAvailableModels(fileName);
    
    if (!models.empty()) {
        // Load main model
        modelManager->LoadModelAs(fileName, models[0], "MainCharacter", device.Get());
        
        // Load additional parts
        for (size_t i = 1; i < models.size(); ++i) {
            std::string partName = "CharacterPart" + std::to_string(i);
            modelManager->LoadModelAs(fileName, models[i], partName, device.Get());
        }
    }
}
```

#### Selective Scene Loading
```cpp
void LoadSceneObjects() {
    // Query all objects in scene file
    auto objects = modelManager->GetAvailableModels(L"scenes/level1.x");
    
    // Only load required objects
    std::vector<std::string> requiredObjects = {"Tree1", "House", "Fence"};
    
    for (const auto& objName : requiredObjects) {
        if (std::find(objects.begin(), objects.end(), objName) != objects.end()) {
            modelManager->LoadModel(L"scenes/level1.x", objName, device.Get());
        }
    }
}
```

## System Refactoring

### Current Refactoring Status (Phase 1 Completed)

The model system underwent significant cleanup in 2025-07-23:

#### Removed Components
- ✅ XFileLoader (replaced by XModelEnhanced)
- ✅ XModelLoaderV2 (header only, no implementation)
- ✅ Loader wrapper class (unnecessary with unified interface)
- ✅ ModelExtractor (unused functionality)
- ✅ V2 model system files (incomplete implementation)

#### Active Components
- **XModelEnhanced**: Primary X file loader with multi-object support
- **FbxLoader**: Full FBX support with texture loading
- **GltfModelLoader**: Complete glTF 2.0 implementation

### Future Refactoring Plans

#### Phase 2: Enhanced Interfaces
```cpp
struct ModelLoadOptions {
  bool loadTextures = true;
  bool loadAnimations = true;
  bool generateTangents = false;
  bool optimizeMeshes = false;
  float scaleFactor = 1.0f;
};

struct IModelLoaderV2 {
  // Single model loading with options
  virtual std::unique_ptr<ModelData> LoadModel(
    const std::filesystem::path& file,
    const std::string& modelName,
    IDirect3DDevice9* device,
    const ModelLoadOptions& options = {}) = 0;
  
  // Capability queries
  virtual ModelCapabilities GetCapabilities() const = 0;
  virtual bool CanLoad(const std::filesystem::path& file) const = 0;
};
```

#### Phase 3: Model Saving
```cpp
struct IModelSaver {
  virtual bool SaveModel(
    const ModelData& model,
    const std::filesystem::path& file,
    const ModelSaveOptions& options = {}) = 0;
  
  virtual bool SaveAll(
    const std::map<std::string, ModelData>& models,
    const std::filesystem::path& file,
    const ModelSaveOptions& options = {}) = 0;
};
```

## Debugging and Common Issues

### Debug Output Analysis

#### Successful X File Loading
```
XModelEnhanced: Found 7 meshes in X file
AllocateHierarchy: Attempting to load texture: HORSE3.BMP ✓
SetTexture successfully loaded texture: Horse4.bmp
Using simple texture shader
DrawWithEffect: Texture set successfully
```

#### Model Statistics
- **test.x**: 1 horse model (horse00)
- **test1.x**: 7 horse models (horse01-horse07)
- **horse_group.x**: 7 horse models (horse01-horse07)
- **rider_group.x**: 6 rider models (Rider01-Rider06)

### Common Issues and Solutions

#### 1. Black Model Rendering
**Problem**: Models render as solid black
**Solution**: 
- Check vertex colors (should be white: 255,255,255)
- Verify texture loading in debug output
- Use simple shader instead of skeletal animation shader for static meshes

#### 2. Missing Textures
**Problem**: Textures not loading with models
**Solution**:
- Ensure textures are in test/ directory
- Check Material structure has textureFileName populated
- Verify `useOriginalTextures` flag is set appropriately

#### 3. Skeletal Animation Shader Issues
**Problem**: Models with bones but no weights render incorrectly
**Solution**:
```cpp
// Detect and handle static meshes with bone definitions
bool hasBoneWeights = CheckForBoneWeights(model);
if (!hasBoneWeights || model->skeleton.joints.empty()) {
    // Use simple shader
    model->mesh.DrawWithEffect(device, simpleTextureEffect_);
} else {
    // Use skeletal animation
    model->mesh.DrawWithAnimation(device, skeletalAnimationEffect_, boneMatrices);
}
```

## Best Practices

### 1. Use AssetManager for Loading
```cpp
// Preferred approach
auto models = assetManager->LoadAllModels("model.fbx");

// Instead of direct loader usage
FbxLoader loader;
auto models = loader.Load("model.fbx", device);
```

### 2. Check File Contents Before Loading
```cpp
// Query available models first
auto available = modelManager->GetAvailableModels(fileName);
if (available.empty()) {
    std::cerr << "No models found in file" << std::endl;
    return;
}
```

### 3. Handle Loading Failures
```cpp
try {
    auto models = modelManager->LoadModels(fileName, device);
    if (models.empty()) {
        // Fallback to default model
        models = modelManager->LoadModels(L"default.x", device);
    }
} catch (const std::exception& e) {
    std::cerr << "Model loading error: " << e.what() << std::endl;
}
```

### 4. Memory Management
- Models are managed with smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Textures are cached in TextureManager to avoid duplicates
- Clear unused models with `modelManager->RemoveModel(name)`

### 5. Performance Optimization
- Load models asynchronously when possible
- Use LOD (Level of Detail) for distant objects
- Consider model instancing for repeated objects
- Optimize mesh data with `optimizeMeshes` option

## Configuration Guidelines

### Camera Settings
```cpp
// Recommended initial values
const float INITIAL_CAMERA_DISTANCE = 50.0f;  // Adjust based on model size
const float MODEL_SCALE = 10.0f;               // For proper visibility
const float MODEL_SPACING = 15.0f;             // For multi-model scenes
```

### Lighting Configuration
```cpp
// Basic lighting setup
D3DLIGHT9 light;
light.Type = D3DLIGHT_DIRECTIONAL;
light.Direction = D3DXVECTOR3(0, -1, 1);
light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
light.Ambient = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
```

### Texture Settings
```cpp
// Texture loading configuration
ModelData::useOriginalTextures = false;  // Override with custom textures
mesh.SetTexture(device, "custom_texture.bmp");
```

## Integration with Modern Architecture

The model system integrates with the engine's modern architecture:

### Service Locator Pattern
```cpp
// Access model system through service locator
auto assetManager = ServiceLocator::GetAssetManager();
auto models = assetManager->LoadAllModels("scene.fbx");
```

### Event System Integration
```cpp
// Notify when models are loaded
EventManager::Publish(ModelLoadedEvent{modelName, success});
```

### Scene Management
```cpp
// Load models for specific scene
void GameScene::Initialize() {
    auto models = services_->GetAssetManager()->LoadAllModels("level1.x");
    for (auto& model : models) {
        AddGameObject(std::make_unique<ModelObject>(model));
    }
}
```

This comprehensive guide provides all the information needed to work with the model loading system effectively, from basic usage to advanced optimization strategies.