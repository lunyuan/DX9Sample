# FBX Texture Loading Implementation

This document records the implementation of FBX texture loading functionality completed on 2025-07-23.

## Problem Background

### Initial Issues
1. X files loaded with textures correctly
2. FBX files rendered as black (no textures)
3. Texture filenames were lost during X→FBX conversion
4. FBX SDK linking errors with static members

### Root Causes Identified
1. `AllocateHierarchy.cpp` was discarding texture filenames (`pTextureFilename = nullptr`)
2. `Material` struct only stored texture pointers, not filenames
3. `FbxLoader` had placeholder code: `material.tex = nullptr` (line 399)
4. FBX SDK static member linking issues with `/MTd` runtime library

## Solution Implementation

### 1. Data Structure Changes

#### Material Structure Enhancement
```cpp
// SkinMesh.h
struct Material {
    D3DMATERIAL9        mat;
    IDirect3DTexture9*  tex = nullptr;
    std::string         textureFileName;  // Added for texture filename preservation
};
```

#### MeshContainerEx Enhancement
```cpp
// XFileTypes.h
struct MeshContainerEx : public D3DXMESHCONTAINER {
    // ... existing members ...
    std::vector<std::string> m_TextureFileNames{};  // Added for texture filenames
};
```

### 2. X File Loader Modifications

#### AllocateHierarchy.cpp
- Now preserves texture filenames in `m_TextureFileNames` vector
- Still creates texture objects but also stores filenames

#### XModelEnhanced.cpp
- Removed unnecessary texture clearing code that was causing black rendering
- Now extracts texture filenames from MeshContainerEx
- Preserves both texture pointers and filenames in Material structure

### 3. FBX Saver Implementation

#### FbxSaver.cpp
- Now exports texture references when `textureFileName` is available
- Creates FbxFileTexture objects and links them to materials
- Debug output confirms texture export: "FbxSaver: Exported texture reference: HORSE3.BMP"

### 4. FBX Loader Implementation

#### FbxLoader.cpp - Final Working Solution
```cpp
// Workaround to avoid FBX SDK static member linking issues
std::string objTypeName = obj->GetClassId().GetName();
if (objTypeName == "FbxFileTexture") {
    FbxFileTexture* fileTexture = static_cast<FbxFileTexture*>(obj);
    if (fileTexture) {
        // Try GetRelativeFileName first, then GetFileName
        const char* fileName = fileTexture->GetRelativeFileName();
        if (!fileName || strlen(fileName) == 0) {
            fileName = fileTexture->GetFileName();
        }
        
        if (fileName && strlen(fileName) > 0) {
            LoadTextureFromFile(fileName, &material.tex, device, fbxFilePath);
        }
    }
}
```

#### LoadTextureFromFile Implementation
Tries multiple strategies to locate texture files:
1. Absolute path (if texture path is absolute)
2. Relative to FBX file directory
3. In test/ directory
4. In current working directory

### 5. Project Configuration

#### DX9Sample.vcxproj
```xml
<PreprocessorDefinitions>
    ...;FBXSDK_STATIC;...
</PreprocessorDefinitions>
<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>
<AdditionalDependencies>
    ...;libfbxsdk.lib;...
</AdditionalDependencies>
```

## Technical Challenges Overcome

### 1. FBX SDK Linking Issues
- **Problem**: Unresolved external symbols for FbxTexture::ClassId, FbxSurfaceMaterial::sDiffuse, etc.
- **Solution**: Avoided template functions and static members, used string comparison and property lookup instead

### 2. Access Violations
- **Problem**: Crash when calling texture->GetFileName()
- **Solution**: Used safer casting methods and exception handling

### 3. Property Name Variations
- **Problem**: "FileName" property not found
- **Solution**: Used GetRelativeFileName() and GetFileName() methods directly on FbxFileTexture object

## Final Result

✅ **X File Loading**: Textures display correctly (horse textures)
✅ **FBX File Loading**: Textures display correctly (brown horses confirmed)
✅ **X to FBX Conversion**: Texture references preserved
✅ **Build Configuration**: Stable with FBX SDK 2020.3.7

## Debug Output Example (Working)
```
=== Step 1: Load X file and save as FBX ===
X Model 0 materials: 2
  Material 0: tex=0000020839B6AC80, filename='HORSE3.BMP'
  Material 1: tex=0000020839B6AB40, filename='RED.BMP'
FbxSaver: Exported texture reference: HORSE3.BMP
FbxSaver: Exported texture reference: RED.BMP

=== Step 2: Load our FBX with texture info ===
FbxLoader: Found FbxFileTexture object
FbxLoader: Found texture file via method: HORSE3.BMP
FbxLoader: Loaded texture from test/: HORSE3.BMP
```

## Usage

### Loading FBX with Textures
```cpp
auto models = assetManager->LoadAllModels("horse_group.fbx");
// Models now load with textures automatically
```

### Converting X to FBX with Texture Preservation
```cpp
// Load X file
auto xModels = assetManager->LoadAllModels("model.x");

// Save as FBX
auto fbxSaver = CreateFbxSaver();
ModelSaveOptions saveOptions;
saveOptions.includeMaterials = true;

std::map<std::string, ModelData> modelMap;
for (size_t i = 0; i < xModels.size(); ++i) {
    modelMap["Model_" + std::to_string(i)] = *xModels[i];
}

fbxSaver->SaveAll(modelMap, "output.fbx", saveOptions);
```

## Future Improvements

1. Implement texture path resolution in FbxSaver to use relative paths
2. Add support for other texture types (normal maps, specular maps)
3. Consider upgrading to dynamic linking (/MD) for easier FBX SDK integration
4. Add texture caching to avoid loading duplicates

## Dependencies

- DirectX 9 SDK
- FBX SDK 2020.3.7
- Visual Studio 2022 with C++20
- Windows 10 SDK