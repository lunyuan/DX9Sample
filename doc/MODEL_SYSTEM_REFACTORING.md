# Model Loading/Saving System Refactoring

## Overview
Complete refactoring of the model loading system to support multiple formats with a unified interface, plus new model saving capabilities.

## New Architecture

### 1. Enhanced Model Loader Interface (IModelLoaderV2)

#### Key Improvements:
- **Loading Options**: Configurable options for texture loading, animations, optimization
- **Single/Multi Model Loading**: Support for files containing multiple models
- **Capability Queries**: Check what features each format supports
- **Validation**: Pre-loading validation and memory estimation
- **Better Error Handling**: Detailed error information

#### Interface Methods:
```cpp
// Load single model with options
LoadModel(file, modelName, device, options)

// Load all models from file
LoadAll(file, device, options)

// Query capabilities
GetCapabilities()
CanLoad(file)
ValidateFile(file)
EstimateMemoryUsage(file)
```

### 2. New Model Saver Interface (IModelSaver)

#### Features:
- **Save single or multiple models**
- **Configurable save options**
- **Format validation**
- **Progress tracking**
- **Texture embedding/external references**

#### Save Options:
- Embed textures
- Compress data
- Include animations
- Optimize meshes
- Metadata (author, copyright)
- Format-specific options

### 3. Enhanced Model Data Structure (ModelDataV2)

#### New Features:
- **Multiple meshes support**
- **Scene hierarchy** (SceneNode tree)
- **LOD levels**
- **Bounding volumes**
- **Rich metadata**
- **Animation clips**
- **Statistics tracking**

#### Structure:
```cpp
ModelDataV2 {
    vector<SkinMesh> meshes;      // Multiple meshes
    Skeleton skeleton;            // Skeletal data
    vector<AnimationClip> animations;
    SceneNode* rootNode;          // Scene hierarchy
    vector<LODLevel> lodLevels;   // Level of detail
    BoundingBox/Sphere bounds;    // Bounding volumes
    ModelMetadata metadata;       // Rich metadata
    Statistics stats;             // Usage statistics
}
```

### 4. Model Format Converter

#### Features:
- **Any-to-any format conversion**
- **Batch processing**
- **Coordinate system conversion**
- **Unit conversion**
- **Progress callbacks**
- **Validation before conversion**

#### Supported Conversions:
- X → glTF/GLB
- FBX → glTF/GLB
- glTF → FBX
- Any → Any (extensible)

## Implementation Status

### Completed:
1. ✅ **IModelLoaderV2** interface design
2. ✅ **IModelSaver** interface design
3. ✅ **ModelDataV2** enhanced structure
4. ✅ **GltfSaver** basic implementation
5. ✅ **ModelFormatConverter** framework

### TODO:
1. ⏳ Refactor existing loaders to IModelLoaderV2
   - XModelLoaderV2
   - FbxLoaderV2
   - GltfLoaderV2
2. ⏳ Implement additional savers
   - FbxSaver
   - XSaver (if needed)
3. ⏳ Implement ModelFormatConverter
4. ⏳ Add unit tests
5. ⏳ Update AssetManager to use new interfaces

## Usage Examples

### Loading with Options:
```cpp
auto loader = CreateGltfLoaderV2();
ModelLoadOptions options;
options.scaleFactor = 0.01f;  // Convert cm to meters
options.generateTangents = true;
options.optimizeMeshes = true;

auto model = loader->LoadModel("character.glb", "Character", device, options);
```

### Saving with Options:
```cpp
auto saver = CreateGltfSaver();
ModelSaveOptions options;
options.embedTextures = true;
options.compressData = true;
options.authorName = "DX9Sample";

auto result = saver->SaveModel(*model, "output.glb", options);
if (result.success) {
    cout << "Saved " << result.bytesWritten << " bytes\n";
}
```

### Format Conversion:
```cpp
ConversionOptions options;
options.loadOptions.scaleFactor = 0.01f;
options.saveOptions.embedTextures = true;
options.generateMissingNormals = true;

auto result = ConvertModel("input.fbx", "output.glb", options);
```

## Benefits

1. **Unified Interface**: Same API for all formats
2. **Extensibility**: Easy to add new formats
3. **Feature Parity**: All formats support same options where possible
4. **Better Error Handling**: Detailed error information
5. **Performance**: Memory estimation, optimization options
6. **Flexibility**: Fine-grained control over loading/saving
7. **Modern C++**: Smart pointers, move semantics, constexpr

## Migration Guide

### For Existing Code:
```cpp
// Old way
XModelLoader loader;
auto models = loader.Load(file, device);

// New way
auto loader = CreateXModelLoaderV2();
auto models = loader->LoadAll(file, device);
```

### Adding New Formats:
1. Implement IModelLoaderV2 interface
2. Implement IModelSaver interface (optional)
3. Register with ModelFormatConverter
4. Add factory functions

## Best Practices

1. **Always check capabilities** before using features
2. **Validate files** before loading for better error handling
3. **Use progress callbacks** for large files
4. **Consider memory usage** with EstimateMemoryUsage()
5. **Specify options explicitly** rather than relying on defaults
6. **Handle warnings** from save operations
7. **Test conversions** with small files first