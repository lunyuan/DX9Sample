# Texture Loading Issue Report

**Date**: 2025-07-23  
**Issue**: Models rendering black (no textures)  
**Status**: Under Investigation

## Problem Timeline

1. **Initial State**: X files loaded with textures correctly (working)
2. **FBX Implementation**: Added FBX texture loading support
3. **First Issue**: FBX files rendered black (textures not loading)
4. **Current Issue**: Both X and FBX files rendering black

## Investigation Summary

### Code Review Findings

#### 1. Texture Loading in X Files (AllocateHierarchy.cpp)
- Correctly loads textures from disk
- Has fallback for RED.BMP → Horse4.bmp
- Debug output shows texture loading attempts
- Code appears intact and functional

#### 2. XModelEnhanced.cpp
- Previously had issue clearing textures when `useOriginalTextures=false`
- This was fixed by removing the texture clearing code
- Current code preserves loaded textures correctly

#### 3. SkinMesh.cpp Rendering
- Correctly calls `SetTexture(0, materials[0].tex)` 
- Debug code added to verify texture pointers
- Render path appears correct

#### 4. FbxLoader.cpp
- Texture loading implemented with multiple path strategies
- Debug output for all loading attempts
- May have issues with texture path resolution

### Potential Root Causes

1. **Texture State Management**
   - Texture stage states might not be set correctly
   - Previous texture state might be interfering

2. **Shader/Effect Issues**
   - Simple texture shader might not be handling textures properly
   - Fixed function pipeline texture setup might be incomplete

3. **Working Directory**
   - Texture paths might be relative to wrong directory
   - AllocateHierarchy uses current directory for texture loading

4. **Material Setup**
   - Materials might have incorrect ambient/diffuse values
   - Texture blend states might be wrong

## Debug Information Added

### GameScene.cpp (Line 327-338)
```cpp
// Output material debug info
if (noAnimDebugCount == 1) {
    char debugMsg2[512];
    sprintf_s(debugMsg2, "Model %zu materials: %zu\n", modelIndex, model->mesh.materials.size());
    OutputDebugStringA(debugMsg2);
    for (size_t matIdx = 0; matIdx < model->mesh.materials.size(); ++matIdx) {
        sprintf_s(debugMsg2, "  Material %zu: tex=%p, filename=%s\n", 
            matIdx, 
            model->mesh.materials[matIdx].tex, 
            model->mesh.materials[matIdx].textureFileName.c_str());
        OutputDebugStringA(debugMsg2);
    }
}
```

### SkinMesh.cpp (Line 375-378)
```cpp
char debugMsg[256];
sprintf_s(debugMsg, "SkinMesh::Draw - Material texture: %p, Current texture: %p, FileName: %s\n", 
          materials[0].tex, currentTex, materials[0].textureFileName.c_str());
OutputDebugStringA(debugMsg);
```

## Texture Files Status

### Required Textures
- `Horse3.bmp` - ✅ Exists in test/
- `RED.BMP` - ❌ Does not exist (uses Horse4.bmp fallback)
- `Horse4.bmp` - ✅ Exists in test/

### X File References
From horse_group.x:
```
TextureFilename {
  "RED.BMP";
}
TextureFilename {
  "HORSE3.BMP";
}
```

## Recent Changes That Could Affect Textures

1. **Phase 1 Cleanup** (Completed)
   - Removed unused components
   - No direct impact on texture loading code
   - Build successful after cleanup

2. **Previous Fix** 
   - Removed texture clearing in XModelEnhanced
   - Should have improved texture loading

## Recommended Next Steps

### 1. Immediate Debugging
- Run with debugger attached to see OutputDebugString messages
- Check if texture pointers are null or valid
- Verify texture stage states

### 2. Test Simple Case
```cpp
// Add to GameScene::Render() temporarily
IDirect3DTexture9* testTex = nullptr;
HRESULT hr = D3DXCreateTextureFromFileA(device, "Horse3.bmp", &testTex);
if (SUCCEEDED(hr)) {
    device->SetTexture(0, testTex);
    // Draw a test quad
    testTex->Release();
}
```

### 3. Check Render States
```cpp
// Ensure these are set
device->SetRenderState(D3DRS_LIGHTING, TRUE);
device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
```

### 4. Compare Working vs Non-Working
- Find last known working commit
- Use git bisect to identify breaking change
- Check render state differences

## Hypothesis

The most likely cause is that texture stage states or render states are not properly configured for the fixed function pipeline after switching between shader and non-shader rendering paths. The texture data is likely loaded correctly but not being applied during rendering.

## Action Items

1. ✅ Add debug output (completed)
2. ⬜ Run with debugger to collect output
3. ⬜ Test simple texture case
4. ⬜ Verify render states
5. ⬜ Check git history for breaking change
6. ⬜ Test with forced simple shader mode

## Conclusion

The issue appears to be a rendering configuration problem rather than a texture loading problem. The cleanup phase is unlikely to be the cause since the issue existed before cleanup (FBX was already black). Focus should be on render state management and the transition between shader and fixed-function rendering paths.