#pragma once
#include <string>
#include <d3d9.h>

// Forward declarations
struct IAssetManager;

// 多模型 X to glTF 轉換函式
bool ConvertXToGltfMultiModel(IDirect3DDevice9* device, IAssetManager* assetManager, 
                              const std::string& xFile, const std::string& gltfFile);