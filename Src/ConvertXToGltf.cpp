#include <iostream>
#include <memory>
#include <d3d9.h>
#include <d3dx9.h>
#include "IAssetManager.h"
#include "AssetManager.h"
#include "IModelSaver.h"
#include "GltfSaver.h"
#include "D3DContext.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "XModelLoader.h"
#include "ModelData.h"

// 將 X 檔案轉換為 glTF 的工具函式
bool ConvertXToGltf(IDirect3DDevice9* device, const std::string& xFile, const std::string& gltfFile) {
    std::cout << "Converting " << xFile << " to " << gltfFile << "..." << std::endl;
    
    // 創建必要的管理器
    auto textureManager = CreateTextureManager(device);
    auto modelLoader = std::make_unique<XModelLoader>();
    auto modelManager = CreateModelManager(std::move(modelLoader), textureManager.get());
    
    // 載入 X 檔案
    try {
        ModelData xModel;
        if (!modelManager->LoadModel(std::wstring(xFile.begin(), xFile.end()), xModel)) {
            std::cerr << "Failed to load X file: " << xFile << std::endl;
            return false;
        }
        
        std::cout << "Successfully loaded X file" << std::endl;
        
        // 創建 glTF saver
        auto gltfSaver = CreateGltfSaver();
        
        // 設定保存選項
        ModelSaveOptions options;
        options.embedTextures = true;  // 嵌入紋理
        options.prettyPrint = true;
        options.author = "DX9Sample Converter";
        
        // 保存為 glTF
        auto result = gltfSaver->SaveModel(xModel, gltfFile, options);
        
        if (result.success) {
            std::cout << "Successfully saved glTF file: " << gltfFile << std::endl;
            std::cout << "File size: " << result.bytesWritten << " bytes" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to save glTF file: " << result.error << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during conversion: " << e.what() << std::endl;
        return false;
    }
}

// 主要轉換函式 - 在 GameScene 中使用
bool ConvertCurrentModelToGltf(IDirect3DDevice9* device) {
    // 轉換 horse_group.x 為 horse_group.gltf
    if (ConvertXToGltf(device, "horse_group.x", "horse_group.gltf")) {
        std::cout << "Conversion completed successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "Conversion failed!" << std::endl;
        return false;
    }
}