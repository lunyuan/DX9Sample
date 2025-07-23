#pragma once
#include <string>
#include <d3d9.h>

// 將 X 檔案轉換為 glTF 的工具函式
bool ConvertXToGltf(IDirect3DDevice9* device, const std::string& xFile, const std::string& gltfFile);

// 主要轉換函式 - 在 GameScene 中使用
bool ConvertCurrentModelToGltf(IDirect3DDevice9* device);