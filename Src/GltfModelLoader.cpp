#include "GltfModelLoader.h"
#include "GltfLoader.h"
#include "ModelData.h"
#include "SkinMesh.h"
#include "Skeleton.h"
#include "tiny_gltf.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <windows.h>

std::map<std::string, ModelData> GltfModelLoader::Load(
    const std::filesystem::path& file, IDirect3DDevice9* device) const {
    
    std::map<std::string, ModelData> models;
    
    
    try {
        // 檢查檔案是否存在
        if (!std::filesystem::exists(file)) {
            return models;
        }
        
        // 載入 glTF 檔案
        tinygltf::TinyGLTF loader;
        tinygltf::Model gltfModel;
        std::string err, warn;
        
        bool isBinary = (file.extension() == ".glb");
        bool ret = isBinary ? 
            loader.LoadBinaryFromFile(&gltfModel, &err, &warn, file.string()) :
            loader.LoadASCIIFromFile(&gltfModel, &err, &warn, file.string());
            
        if (!ret) {
            return models;
        }
        
        // 處理每個網格
        for (size_t meshIdx = 0; meshIdx < gltfModel.meshes.size(); ++meshIdx) {
            const auto& mesh = gltfModel.meshes[meshIdx];
            
            // 處理每個 primitive
            for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
                const auto& prim = mesh.primitives[primIdx];
                
                ModelData modelData;
                
                // 檢查是否有必要的屬性
                if (prim.attributes.find("POSITION") == prim.attributes.end()) {
                    continue;
                }
                
                // 載入頂點位置
                const auto& posAccessor = gltfModel.accessors[prim.attributes.at("POSITION")];
                const auto& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
                const auto& posBuffer = gltfModel.buffers[posBufferView.buffer];
                
                size_t vertexCount = posAccessor.count;
                modelData.mesh.vertices.resize(vertexCount);
                
                const float* posPtr = reinterpret_cast<const float*>(
                    &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
                    
                for (size_t i = 0; i < vertexCount; ++i) {
                    modelData.mesh.vertices[i].pos.x = posPtr[3 * i + 0];
                    modelData.mesh.vertices[i].pos.y = posPtr[3 * i + 1];
                    modelData.mesh.vertices[i].pos.z = posPtr[3 * i + 2];
                    
                    // 設置預設的白色頂點顏色
                    modelData.mesh.vertices[i].col = D3DCOLOR_XRGB(255, 255, 255);
                }
                
                // 載入法線
                if (prim.attributes.find("NORMAL") != prim.attributes.end()) {
                    const auto& normAccessor = gltfModel.accessors[prim.attributes.at("NORMAL")];
                    const auto& normBufferView = gltfModel.bufferViews[normAccessor.bufferView];
                    const auto& normBuffer = gltfModel.buffers[normBufferView.buffer];
                    
                    const float* normPtr = reinterpret_cast<const float*>(
                        &normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset]);
                        
                    for (size_t i = 0; i < vertexCount; ++i) {
                        modelData.mesh.vertices[i].norm.x = normPtr[3 * i + 0];
                        modelData.mesh.vertices[i].norm.y = normPtr[3 * i + 1];
                        modelData.mesh.vertices[i].norm.z = normPtr[3 * i + 2];
                    }
                }
                
                // 載入紋理座標
                if (prim.attributes.find("TEXCOORD_0") != prim.attributes.end()) {
                    const auto& uvAccessor = gltfModel.accessors[prim.attributes.at("TEXCOORD_0")];
                    const auto& uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
                    const auto& uvBuffer = gltfModel.buffers[uvBufferView.buffer];
                    
                    const float* uvPtr = reinterpret_cast<const float*>(
                        &uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);
                        
                    for (size_t i = 0; i < vertexCount; ++i) {
                        modelData.mesh.vertices[i].uv.x = uvPtr[2 * i + 0];
                        modelData.mesh.vertices[i].uv.y = uvPtr[2 * i + 1];
                    }
                }
                
                // 載入索引
                if (prim.indices >= 0) {
                    const auto& idxAccessor = gltfModel.accessors[prim.indices];
                    const auto& idxBufferView = gltfModel.bufferViews[idxAccessor.bufferView];
                    const auto& idxBuffer = gltfModel.buffers[idxBufferView.buffer];
                    
                    size_t indexCount = idxAccessor.count;
                    modelData.mesh.indices.resize(indexCount);
                    
                    if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const unsigned short* idxPtr = reinterpret_cast<const unsigned short*>(
                            &idxBuffer.data[idxBufferView.byteOffset + idxAccessor.byteOffset]);
                        for (size_t i = 0; i < indexCount; ++i) {
                            modelData.mesh.indices[i] = idxPtr[i];
                        }
                    } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        const unsigned int* idxPtr = reinterpret_cast<const unsigned int*>(
                            &idxBuffer.data[idxBufferView.byteOffset + idxAccessor.byteOffset]);
                        for (size_t i = 0; i < indexCount; ++i) {
                            modelData.mesh.indices[i] = idxPtr[i];
                        }
                    }
                }
                
                // 處理材質和貼圖
                if (prim.material >= 0 && prim.material < static_cast<int>(gltfModel.materials.size())) {
                    const auto& material = gltfModel.materials[prim.material];
                    
                    // 創建材質
                    Material modelMat;
                    
                    // 設置材質屬性
                    const auto& pbr = material.pbrMetallicRoughness;
                    if (pbr.baseColorFactor.size() >= 4) {
                        modelMat.mat.Diffuse.r = static_cast<float>(pbr.baseColorFactor[0]);
                        modelMat.mat.Diffuse.g = static_cast<float>(pbr.baseColorFactor[1]);
                        modelMat.mat.Diffuse.b = static_cast<float>(pbr.baseColorFactor[2]);
                        modelMat.mat.Diffuse.a = static_cast<float>(pbr.baseColorFactor[3]);
                    }
                    modelMat.mat.Ambient = modelMat.mat.Diffuse;
                    modelMat.mat.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
                    modelMat.mat.Power = 10.0f;
                    
                    // 處理貼圖
                    if (pbr.baseColorTexture.index >= 0 && 
                        pbr.baseColorTexture.index < static_cast<int>(gltfModel.textures.size())) {
                        
                        const auto& texture = gltfModel.textures[pbr.baseColorTexture.index];
                        if (texture.source >= 0 && texture.source < static_cast<int>(gltfModel.images.size())) {
                            const auto& image = gltfModel.images[texture.source];
                            
                            // 取得貼圖檔案名稱
                            std::string textureFileName = image.uri;
                            modelMat.textureFileName = textureFileName;
                            
                            char debugMsg[256];
                            sprintf_s(debugMsg, "GltfModelLoader: Found texture '%s' for material %d\n",
                                      textureFileName.c_str(), prim.material);
                            OutputDebugStringA(debugMsg);
                        }
                    }
                    
                    modelData.mesh.materials.push_back(modelMat);
                } else {
                    // 沒有材質，創建預設材質
                    Material defaultMat;
                    defaultMat.mat.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
                    defaultMat.mat.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
                    defaultMat.mat.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
                    defaultMat.mat.Power = 10.0f;
                    modelData.mesh.materials.push_back(defaultMat);
                }
                
                // 創建 Direct3D 緩衝區
                if (modelData.mesh.CreateBuffers(device)) {
                    // 載入貼圖（如果有的話）
                    if (!modelData.mesh.materials.empty() && !modelData.mesh.materials[0].textureFileName.empty()) {
                        modelData.mesh.SetTexture(device, modelData.mesh.materials[0].textureFileName);
                    }
                    
                    // 產生模型名稱
                    std::string modelName = mesh.name;
                    if (modelName.empty()) {
                        modelName = "Mesh_" + std::to_string(meshIdx);
                    }
                    if (mesh.primitives.size() > 1) {
                        modelName += "_" + std::to_string(primIdx);
                    }
                    
                    models[modelName] = std::move(modelData);
                }
            }
        }
        
    } catch (const std::exception& e) {
        // 錯誤處理
    }
    
    
    return models;
}

std::vector<std::string> GltfModelLoader::GetModelNames(
    const std::filesystem::path& file) const {
    
    std::vector<std::string> names;
    
    try {
        // 載入 glTF 檔案以獲取模型名稱
        tinygltf::TinyGLTF loader;
        tinygltf::Model gltfModel;
        std::string err, warn;
        
        bool isBinary = (file.extension() == ".glb");
        bool ret = isBinary ? 
            loader.LoadBinaryFromFile(&gltfModel, &err, &warn, file.string()) :
            loader.LoadASCIIFromFile(&gltfModel, &err, &warn, file.string());
            
        if (ret) {
            for (size_t meshIdx = 0; meshIdx < gltfModel.meshes.size(); ++meshIdx) {
                const auto& mesh = gltfModel.meshes[meshIdx];
                
                for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
                    std::string modelName = mesh.name;
                    if (modelName.empty()) {
                        modelName = "Mesh_" + std::to_string(meshIdx);
                    }
                    if (mesh.primitives.size() > 1) {
                        modelName += "_" + std::to_string(primIdx);
                    }
                    names.push_back(modelName);
                }
            }
        }
    } catch (...) {
        // 錯誤處理
    }
    
    return names;
}