# Texture Loading Strategy

Date: 2025-01-22

## Requirements

1. **Default Behavior**: Do NOT load textures from model files by default
2. **Configuration**: Add option to control whether to use model textures
3. **Priority**: If SetTexture is called, always use that texture over model textures

## Current Issue

The .x files contain texture references:
- HORSE3.BMP (exists)
- RED.BMP (does not exist, fallback to Horse4.bmp)

But user wants to use Horse4.bmp for all horses by default, not the textures specified in the model files.

## Implementation Plan

### Option 1: Add flag to AssetManager
- Add `bool useModelTextures = false` to AssetManager
- Pass this flag to loaders
- Only load textures if flag is true

### Option 2: Add flag to ModelData
- Add `bool useOriginalTextures = false` to ModelData
- Clear textures after loading if false
- Allow SetTexture to override

### Option 3: Modify AllocateHierarchy
- Add parameter to control texture loading
- Skip texture loading entirely if disabled

## Chosen Solution

Implement Option 2 - Add flag to ModelData:
- Most flexible
- Allows per-model control
- Easy to implement SetTexture override

## Code Changes

1. Update ModelData structure
2. Modify XModelEnhanced to respect flag
3. Ensure SetTexture always works
4. Update GameScene to use SetTexture with Horse4.bmp