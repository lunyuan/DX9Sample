# Model Formats Reference

## Overview

This reference document provides comprehensive technical details about the three 3D model formats supported by the DX9Sample engine: DirectX .X files, Autodesk FBX files, and glTF 2.0 files. Each format has unique characteristics, implementation details, and conversion capabilities.

## DirectX .X Format

### Format Overview
- **Extension**: .x
- **Type**: DirectX native format (deprecated but still supported)
- **Structure**: Text or binary format with frame hierarchy
- **Support Level**: Full support with enhanced multi-object loading

### Implementation Details

#### XModelEnhanced Loader
The enhanced X file loader provides complete support for multi-object X files:

```cpp
// Loading process
1. D3DXLoadMeshHierarchyFromX loads entire frame hierarchy
2. Traverse frames recursively to collect all meshes
3. Each mesh becomes a separate ModelData object
4. Apply frame transformations to vertices
5. Extract materials and skeleton structure
```

#### Multi-Object Support
X files can contain multiple objects as separate frames:
- **test.x**: 1 horse model (horse00)
- **test1.x**: 7 horse models (horse01-horse07)
- **horse_group.x**: 7 horse models (horse01-horse07)
- **rider_group.x**: 6 rider models (Rider01-Rider06)

#### Texture Handling
```cpp
// AllocateHierarchy.cpp preserves texture filenames
if (pTextureFilename != nullptr) {
    meshContainer->m_TextureFileNames.push_back(pTextureFilename);
}

// Material structure includes texture filename
struct Material {
    D3DMATERIAL9        mat;
    IDirect3DTexture9*  tex = nullptr;
    std::string         textureFileName;  // Preserved for export
};
```

#### Common Issues and Solutions

**Issue: Models render black with skeletal animation shader**
- **Cause**: X file contains bone definitions but no bone weights
- **Solution**: Use simple texture shader for static meshes
```cpp
// Detect static mesh with bones
if (model->skeleton.joints.size() > 0 && !HasBoneWeights(model)) {
    // Use simple shader instead of skeletal animation
    model->mesh.DrawWithEffect(device, simpleTextureEffect_);
}
```

**Issue: Missing texture (RED.BMP not found)**
- **Solution**: Fallback texture loading
```cpp
// Automatic fallback to alternative texture
if (!FileExists("RED.BMP")) {
    LoadTexture("Horse4.bmp");  // Use fallback texture
}
```

### X File Structure Analysis
```
Frame x3ds_horse01 {
  FrameTransformMatrix {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0;;
  }
  Mesh {
    349;  // Vertex count
    // Vertex data...
    586;  // Triangle count
    // Face data...
    MeshMaterialList {
      2;  // Material count
      // Material assignments...
    }
  }
}
```

## FBX Format

### Format Overview
- **Extension**: .fbx
- **Type**: Autodesk proprietary format
- **Structure**: Scene graph with nodes, meshes, materials
- **Support Level**: Full support including textures and animations

### Implementation Details

#### FBX Texture Loading (Completed 2025-07-23)
The FBX loader now fully supports texture loading with multiple fallback strategies:

```cpp
// Texture loading workaround for static linking issues
std::string objTypeName = obj->GetClassId().GetName();
if (objTypeName == "FbxFileTexture") {
    FbxFileTexture* fileTexture = static_cast<FbxFileTexture*>(obj);
    const char* fileName = fileTexture->GetRelativeFileName();
    if (!fileName || strlen(fileName) == 0) {
        fileName = fileTexture->GetFileName();
    }
    LoadTextureFromFile(fileName, &material.tex, device, fbxFilePath);
}
```

#### Texture Path Resolution
The loader tries multiple strategies to locate textures:
1. Absolute path (if texture path is absolute)
2. Relative to FBX file directory
3. In test/ directory
4. In current working directory

#### FBX SDK Configuration
```xml
<!-- Project settings for FBX SDK 2020.3.7 -->
<PreprocessorDefinitions>
    FBXSDK_STATIC;  <!-- Static linking -->
</PreprocessorDefinitions>
<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>
<AdditionalDependencies>
    libfbxsdk.lib;
</AdditionalDependencies>
```

#### Technical Challenges Overcome
1. **Static Member Linking Issues**
   - Problem: Unresolved externals for FbxTexture::ClassId
   - Solution: Use string comparison instead of static members

2. **Property Access**
   - Problem: Template property access causes linking errors
   - Solution: Direct method calls on FbxFileTexture object

### FBX Saving Implementation
```cpp
// Export texture references
if (!mat.textureFileName.empty()) {
    FbxFileTexture* texture = FbxFileTexture::Create(scene, "");
    texture->SetFileName(mat.textureFileName.c_str());
    texture->SetRelativeFileName(mat.textureFileName.c_str());
    
    FbxSurfaceLambert* material = FbxSurfaceLambert::Create(scene, matName.c_str());
    material->Diffuse.ConnectSrcObject(texture);
}
```

## glTF 2.0 Format

### Format Overview
- **Extension**: .gltf (JSON) or .glb (binary)
- **Type**: Open standard by Khronos Group
- **Structure**: JSON with external binary buffers
- **Support Level**: Full support with multi-model and texture loading

### Implementation Details

#### Complete glTF Implementation (2025-07-23)
The glTF implementation provides full support for loading and saving:

```cpp
// GltfModelLoader implementation
1. Parse glTF JSON structure
2. Load binary buffer data
3. Extract vertex attributes (position, normal, texcoord)
4. Read material properties including textures
5. Create DirectX 9 compatible mesh data
6. Apply textures automatically
```

#### Multi-Model Support
Successfully tested with horse_group.gltf containing 7 separate horse models:
```cpp
// Each mesh in glTF becomes a separate ModelData
for (const auto& mesh : gltfModel.meshes) {
    ModelData modelData;
    // Process each primitive
    for (const auto& primitive : mesh.primitives) {
        // Extract vertex data and materials
    }
    models[mesh.name] = modelData;
}
```

#### Texture Implementation
```cpp
// Material texture extraction
if (pbr.baseColorTexture.index >= 0) {
    const auto& texture = gltfModel.textures[pbr.baseColorTexture.index];
    const auto& image = gltfModel.images[texture.source];
    modelMat.textureFileName = image.uri;
}

// Automatic texture application
if (!modelData.mesh.materials[0].textureFileName.empty()) {
    modelData.mesh.SetTexture(device, modelData.mesh.materials[0].textureFileName);
}
```

#### Vertex Color Fix
```cpp
// Set vertex colors to white to prevent black rendering
D3DCOLOR white = D3DCOLOR_ARGB(255, 255, 255, 255);
for (auto& vertex : vertices) {
    vertex.color = white;
}
```

### glTF Structure Example
```json
{
  "asset": {"version": "2.0"},
  "scene": 0,
  "scenes": [{"nodes": [0,1,2,3,4,5,6]}],
  "nodes": [
    {"mesh": 0, "name": "horse01"},
    {"mesh": 1, "name": "horse02"}
  ],
  "meshes": [
    {
      "name": "horse01",
      "primitives": [{
        "attributes": {
          "POSITION": 0,
          "NORMAL": 1,
          "TEXCOORD_0": 2
        },
        "indices": 3,
        "material": 0
      }]
    }
  ],
  "materials": [{
    "name": "HorseMaterial",
    "pbrMetallicRoughness": {
      "baseColorTexture": {"index": 0}
    }
  }],
  "textures": [{"source": 0}],
  "images": [{"uri": "HORSE3.BMP"}]
}
```

## Format Conversion

### X to FBX Conversion

#### Process
1. Load X file using XModelEnhanced
2. Create FBX scene with FbxSaver
3. Export mesh data and materials
4. Preserve texture references

#### Implementation
```cpp
// Load X file
auto xModels = assetManager->LoadAllModels("model.x");

// Save as FBX with textures
auto fbxSaver = CreateFbxSaver();
ModelSaveOptions saveOptions;
saveOptions.includeMaterials = true;

std::map<std::string, ModelData> modelMap;
for (size_t i = 0; i < xModels.size(); ++i) {
    modelMap["Model_" + std::to_string(i)] = *xModels[i];
}

fbxSaver->SaveAll(modelMap, "output.fbx", saveOptions);
```

### X to glTF Conversion

#### MultiModelGltfConverter Implementation
Converts all models from X file to glTF with full material support:

```cpp
void ConvertXToGltfMultiModel(device, assetManager, inputFile, outputFile) {
    // Load all models from X file
    auto xModels = assetManager->LoadAllModels(inputFile);
    
    // Create glTF structure
    tinygltf::Model gltfModel;
    
    // Convert each model
    for (const auto& xModel : xModels) {
        // Create mesh, materials, textures
        // Add to glTF model
    }
    
    // Save glTF file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&gltfModel, outputFile, false, true, true, false);
}
```

#### Texture Export
```cpp
// Export texture references in glTF
if (!xMat.textureFileName.empty()) {
    tinygltf::Image image;
    image.uri = xMat.textureFileName;
    gltfModel.images.push_back(image);
    
    tinygltf::Texture texture;
    texture.source = imageIdx;
    gltfModel.textures.push_back(texture);
    
    material.pbrMetallicRoughness.baseColorTexture.index = textureIdx;
}
```

### Conversion Results
- **Input**: horse_group.x with 7 models and textures
- **Output**: horse_group_textured.gltf with:
  - 7 meshes (horse models)
  - 2 images (HORSE3.BMP, Horse10.bmp)
  - 2 textures referencing the images
  - 14 materials with baseColorTexture references

## Format Comparison

| Feature | DirectX .X | FBX | glTF 2.0 |
|---------|------------|-----|----------|
| **Multi-Model** | ✅ Via frames | ✅ Scene graph | ✅ Multiple meshes |
| **Textures** | ✅ Material list | ✅ Full support | ✅ URI references |
| **Animation** | ⚠️ Limited | ✅ Full skeletal | ✅ Full support |
| **PBR Materials** | ❌ | ⚠️ Limited | ✅ Native |
| **File Size** | Small | Large | Medium |
| **Human Readable** | ✅ Text option | ❌ Binary | ✅ JSON |
| **Tool Support** | ❌ Deprecated | ✅ Industry standard | ✅ Growing |
| **Web Compatible** | ❌ | ❌ | ✅ |

## Best Practices by Format

### DirectX .X Files
- Use for legacy content only
- Prefer XModelEnhanced loader for multi-object support
- Handle missing textures with fallbacks
- Use simple shader for static meshes with bones

### FBX Files
- Preferred for content creation pipeline
- Ensure textures are in relative paths
- Use FBX 2020.3.7 SDK for compatibility
- Test texture loading with debug output

### glTF Files
- Preferred for modern applications
- Use for web deployment
- Keep textures as external files for easier updates
- Set vertex colors to white for proper rendering

## Troubleshooting Guide

### General Issues

#### Models Not Loading
1. Check file path is correct
2. Verify file format is supported
3. Check console for error messages
4. Ensure DirectX device is valid

#### Textures Not Displaying
1. Verify texture files exist in expected locations
2. Check material has textureFileName set
3. Ensure useOriginalTextures flag is appropriate
4. Look for texture loading messages in debug output

### Format-Specific Issues

#### X Files
- **Black rendering**: Use simple shader for static meshes
- **Missing RED.BMP**: Expected, uses fallback texture
- **Multiple objects not separating**: Use XModelEnhanced loader

#### FBX Files
- **Linking errors**: Ensure FBXSDK_STATIC is defined
- **Texture paths**: Try relative and absolute paths
- **SDK version**: Use FBX SDK 2020.3.7

#### glTF Files
- **Black models**: Set vertex colors to white
- **Missing textures**: Check image URIs in JSON
- **Multiple models**: Each mesh becomes separate ModelData

This comprehensive reference provides all technical details needed to work with the supported 3D model formats effectively.