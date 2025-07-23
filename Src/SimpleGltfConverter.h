#pragma once
#include <string>
#include <d3d9.h>

// Forward declarations
struct IAssetManager;

// 簡單的 X to glTF 轉換函式
bool SimpleConvertXToGltf(IDirect3DDevice9* device, const std::string& xFile, const std::string& gltfFile);

// 使用現有 AssetManager 的轉換函式
bool SimpleConvertXToGltfWithAssetManager(IDirect3DDevice9* device, IAssetManager* assetManager, 
                                         const std::string& xFile, const std::string& gltfFile);

// 改進的轉換函式 - 支援多個模型
bool SimpleConvertXToGltfV2(IDirect3DDevice9* device, IAssetManager* assetManager, 
                            const std::string& xFile, const std::string& gltfFile);