# Model Loader Refactoring Plan

Date: 2025-01-22

## Current State Analysis

### Model Loaders in Codebase

1. **XModelEnhanced + XModelEnhancedLoader** (ACTIVE)
   - Used by AssetManager for .x files
   - Implements IModelLoader interface  
   - Properly handles multi-object separation
   - Loads textures from materials

2. **FbxLoader** (ACTIVE)
   - Used by AssetManager for .fbx files
   - Implements IModelLoader interface
   - Working correctly

3. **XModelLoader** (PARTIALLY ACTIVE)
   - Implements IModelLoader interface
   - Used by EngineContext to create ModelManager
   - But actual loading is done by AssetManager using XModelEnhancedLoader

4. **XFileLoader** (INACTIVE - TO BE REMOVED)
   - Standalone loader, doesn't implement IModelLoader
   - Has hardcoded search for "x3ds_horse05" (lines 333-344)
   - Only referenced in comments and Loader.cpp wrapper
   - Functionality replaced by XModelEnhanced

5. **XModelLoaderV2** (INACTIVE - TO BE REMOVED)
   - Only header file exists, no implementation
   - Uses different interface (IModelLoaderV2)
   - Not used anywhere

6. **Loader** (INACTIVE - TO BE REMOVED)
   - Wrapper class that calls XFileLoader and GltfLoader
   - Not necessary with unified IModelLoader interface

7. **GltfLoader** (INACTIVE - NEEDS UPDATE)
   - Doesn't implement IModelLoader interface
   - Needs to be updated to follow the pattern

## Refactoring Tasks

### Phase 1: Remove Unused Code (COMPLETED)
- [x] Remove XFileLoader.h/cpp
- [x] Remove XModelLoaderV2.h  
- [x] Remove Loader.h/cpp
- [x] Update project file to remove these files
- [x] Remove references in main.cpp comments

### Phase 2: Unify Architecture
- [ ] Update GltfLoader to implement IModelLoader interface
- [ ] Consider replacing XModelLoader with XModelEnhancedLoader in EngineContext
- [ ] Or remove ModelManager entirely and use AssetManager everywhere

### Phase 3: Fix Texture Loading Issues
- [ ] Verify XModelEnhanced loads textures correctly from .x files
- [ ] Check UV coordinates are properly loaded
- [ ] Ensure texture state is set correctly during rendering

## Current Loading Flow

```
AssetManager::LoadAllModels(filename)
├── .x files → XModelEnhancedLoader → XModelEnhanced::LoadWithSeparation
├── .fbx files → FbxLoader
└── .gltf files → GltfLoader (needs IModelLoader implementation)
```

## Files to Update
1. DX9Sample.vcxproj - remove unused files
2. AssetManager.cpp - already updated to use unified interface
3. EngineContext.cpp - consider updating to use AssetManager
4. main.cpp - remove commented XFileLoader references
5. CLAUDE.md - update architecture documentation

## Risks and Considerations
- Ensure no other code depends on removed loaders
- Test that .x file loading still works after refactoring
- Verify texture loading is not broken
- Keep backup of removed code in case needed