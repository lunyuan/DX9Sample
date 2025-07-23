#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>
#include <d3d9.h>
#include <d3dx9.h>
#include "IAssetManager.h"
#include "AssetManager.h"
#include "D3DContext.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "XModelLoader.h"
#include "ModelData.h"
#include "tiny_gltf.h"
#include <algorithm>

// Prevent min/max macro conflicts
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// 改進的 X to glTF 轉換函式 - 支援多個模型
bool SimpleConvertXToGltfV2(IDirect3DDevice9* device, IAssetManager* assetManager, 
                            const std::string& xFile, const std::string& gltfFile) {
    std::cout << "SimpleConvertXToGltfV2: Converting " << xFile << " to " << gltfFile << "..." << std::endl;
    
    // 添加檔案輸出以便調試
    std::ofstream debugLog("gltf_convert_debug.txt", std::ios::app);
    debugLog << "\n=== SimpleConvertXToGltfV2 called ===" << std::endl;
    debugLog << "Device: " << (device ? "valid" : "null") << std::endl;
    debugLog << "AssetManager: " << (assetManager ? "valid" : "null") << std::endl;
    
    // 載入 X 檔案
    try {
        debugLog << "Loading X file: " << xFile << std::endl;
        
        // 檢查檔案是否存在
        if (!std::filesystem::exists(xFile)) {
            debugLog << "ERROR: X file does not exist: " << xFile << std::endl;
            debugLog.close();
            return false;
        }
        
        auto models = assetManager->LoadAllModels(xFile);
        if (models.empty()) {
            std::cerr << "Failed to load X file: " << xFile << std::endl;
            debugLog << "ERROR: Failed to load X file - models empty" << std::endl;
            debugLog.close();
            return false;
        }
        debugLog << "Loaded " << models.size() << " models from X file" << std::endl;
        
        // 計算所有模型的總頂點和索引數
        size_t totalVertices = 0;
        size_t totalIndices = 0;
        for (const auto& model : models) {
            totalVertices += model->mesh.vertices.size();
            totalIndices += model->mesh.indices.size();
        }
        
        std::cout << "Successfully loaded X file with " << models.size() << " models" << std::endl;
        std::cout << "Total vertices: " << totalVertices << ", Total indices: " << totalIndices << std::endl;
        debugLog << "Total models: " << models.size() << std::endl;
        debugLog << "Total vertices: " << totalVertices << std::endl;
        debugLog << "Total indices: " << totalIndices << std::endl;
        
        // 創建 glTF 模型
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltf;
        
        // 設置資產信息
        gltfModel.asset.version = "2.0";
        gltfModel.asset.generator = "DX9Sample SimpleGltfConverterV2";
        
        // 創建場景
        tinygltf::Scene scene;
        scene.name = "Scene";
        
        // 創建一個大緩衝區來存放所有模型的資料
        std::vector<uint8_t> allBufferData;
        size_t currentBufferOffset = 0;
        
        // 為每個模型創建節點和網格
        for (size_t modelIdx = 0; modelIdx < models.size(); ++modelIdx) {
            const auto& xModel = *models[modelIdx];
            
            debugLog << "Processing model " << modelIdx << " with " 
                     << xModel.mesh.vertices.size() << " vertices" << std::endl;
            
            // 創建節點
            tinygltf::Node node;
            node.name = "Model_" + std::to_string(modelIdx);
            
            // 創建網格
            tinygltf::Mesh mesh;
            mesh.name = node.name;
            
            // 創建基本圖元
            tinygltf::Primitive primitive;
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            
            // 頂點數據
            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<float> texcoords;
            
            // 轉換頂點數據
            for (const auto& vertex : xModel.mesh.vertices) {
                positions.push_back(vertex.pos.x);
                positions.push_back(vertex.pos.y);
                positions.push_back(vertex.pos.z);
                
                normals.push_back(vertex.norm.x);
                normals.push_back(vertex.norm.y);
                normals.push_back(vertex.norm.z);
                
                texcoords.push_back(vertex.uv.x);
                texcoords.push_back(vertex.uv.y);
            }
            
            // 記錄當前模型資料在緩衝區中的偏移
            size_t posOffset = currentBufferOffset;
            size_t posSize = positions.size() * sizeof(float);
            allBufferData.resize(allBufferData.size() + posSize);
            memcpy(allBufferData.data() + posOffset, positions.data(), posSize);
            currentBufferOffset += posSize;
            
            size_t normOffset = currentBufferOffset;
            size_t normSize = normals.size() * sizeof(float);
            allBufferData.resize(allBufferData.size() + normSize);
            memcpy(allBufferData.data() + normOffset, normals.data(), normSize);
            currentBufferOffset += normSize;
            
            size_t uvOffset = currentBufferOffset;
            size_t uvSize = texcoords.size() * sizeof(float);
            allBufferData.resize(allBufferData.size() + uvSize);
            memcpy(allBufferData.data() + uvOffset, texcoords.data(), uvSize);
            currentBufferOffset += uvSize;
            
            size_t indexOffset = currentBufferOffset;
            size_t indexSize = xModel.mesh.indices.size() * sizeof(uint32_t);
            allBufferData.resize(allBufferData.size() + indexSize);
            memcpy(allBufferData.data() + indexOffset, xModel.mesh.indices.data(), indexSize);
            currentBufferOffset += indexSize;
            
            // 創建緩衝視圖索引
            size_t baseBufferViewIdx = gltfModel.bufferViews.size();
            
            // 位置緩衝視圖
            tinygltf::BufferView posView;
            posView.buffer = 0;
            posView.byteOffset = posOffset;
            posView.byteLength = posSize;
            posView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(posView);
            
            // 法線緩衝視圖
            tinygltf::BufferView normView;
            normView.buffer = 0;
            normView.byteOffset = normOffset;
            normView.byteLength = normSize;
            normView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(normView);
            
            // UV緩衝視圖
            tinygltf::BufferView uvView;
            uvView.buffer = 0;
            uvView.byteOffset = uvOffset;
            uvView.byteLength = uvSize;
            uvView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(uvView);
            
            // 索引緩衝視圖
            tinygltf::BufferView indexView;
            indexView.buffer = 0;
            indexView.byteOffset = indexOffset;
            indexView.byteLength = indexSize;
            indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(indexView);
            
            // 創建存取器索引
            size_t baseAccessorIdx = gltfModel.accessors.size();
            
            // 位置存取器
            tinygltf::Accessor posAccessor;
            posAccessor.bufferView = baseBufferViewIdx;
            posAccessor.byteOffset = 0;
            posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            posAccessor.count = positions.size() / 3;
            posAccessor.type = TINYGLTF_TYPE_VEC3;
            
            // 計算邊界
            for (size_t i = 0; i < positions.size() / 3; ++i) {
                float x = positions[i * 3];
                float y = positions[i * 3 + 1];
                float z = positions[i * 3 + 2];
                
                if (i == 0) {
                    posAccessor.minValues = {x, y, z};
                    posAccessor.maxValues = {x, y, z};
                } else {
                    posAccessor.minValues[0] = std::min(posAccessor.minValues[0], (double)x);
                    posAccessor.minValues[1] = std::min(posAccessor.minValues[1], (double)y);
                    posAccessor.minValues[2] = std::min(posAccessor.minValues[2], (double)z);
                    posAccessor.maxValues[0] = std::max(posAccessor.maxValues[0], (double)x);
                    posAccessor.maxValues[1] = std::max(posAccessor.maxValues[1], (double)y);
                    posAccessor.maxValues[2] = std::max(posAccessor.maxValues[2], (double)z);
                }
            }
            gltfModel.accessors.push_back(posAccessor);
            
            // 法線存取器
            tinygltf::Accessor normAccessor;
            normAccessor.bufferView = baseBufferViewIdx + 1;
            normAccessor.byteOffset = 0;
            normAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            normAccessor.count = normals.size() / 3;
            normAccessor.type = TINYGLTF_TYPE_VEC3;
            gltfModel.accessors.push_back(normAccessor);
            
            // UV存取器
            tinygltf::Accessor uvAccessor;
            uvAccessor.bufferView = baseBufferViewIdx + 2;
            uvAccessor.byteOffset = 0;
            uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            uvAccessor.count = texcoords.size() / 2;
            uvAccessor.type = TINYGLTF_TYPE_VEC2;
            gltfModel.accessors.push_back(uvAccessor);
            
            // 索引存取器
            tinygltf::Accessor indexAccessor;
            indexAccessor.bufferView = baseBufferViewIdx + 3;
            indexAccessor.byteOffset = 0;
            indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
            indexAccessor.count = xModel.mesh.indices.size();
            indexAccessor.type = TINYGLTF_TYPE_SCALAR;
            gltfModel.accessors.push_back(indexAccessor);
            
            // 設置圖元屬性
            primitive.attributes["POSITION"] = baseAccessorIdx;
            primitive.attributes["NORMAL"] = baseAccessorIdx + 1;
            primitive.attributes["TEXCOORD_0"] = baseAccessorIdx + 2;
            primitive.indices = baseAccessorIdx + 3;
            
            // 設置材質
            if (modelIdx == 0) {
                // 創建一個共用材質
                tinygltf::Material material;
                material.name = "Material";
                material.pbrMetallicRoughness.baseColorFactor = {1.0, 1.0, 1.0, 1.0};
                material.pbrMetallicRoughness.metallicFactor = 0.0;
                material.pbrMetallicRoughness.roughnessFactor = 0.5;
                material.doubleSided = true;
                gltfModel.materials.push_back(material);
            }
            primitive.material = 0;
            
            // 組裝網格和節點
            mesh.primitives.push_back(primitive);
            gltfModel.meshes.push_back(mesh);
            
            node.mesh = modelIdx;
            scene.nodes.push_back(modelIdx);
            gltfModel.nodes.push_back(node);
            
            debugLog << "Added model " << modelIdx << " to glTF" << std::endl;
        }
        
        // 創建單一緩衝區
        tinygltf::Buffer buffer;
        buffer.data = allBufferData;
        gltfModel.buffers.push_back(buffer);
        
        // 設置場景
        gltfModel.scenes.push_back(scene);
        gltfModel.defaultScene = 0;
        
        // 寫入檔案
        bool ret = gltf.WriteGltfSceneToFile(&gltfModel, gltfFile, 
                                            true, // embedImages
                                            true, // embedBuffers
                                            true, // prettyPrint
                                            false); // writeBinary (false for .gltf, true for .glb)
        
        if (ret) {
            std::cout << "Successfully wrote glTF file: " << gltfFile << std::endl;
            debugLog << "Successfully wrote glTF file with " << models.size() << " models" << std::endl;
        } else {
            std::cerr << "Failed to write glTF file" << std::endl;
            debugLog << "Failed to write glTF file" << std::endl;
        }
        
        debugLog.close();
        return ret;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during conversion: " << e.what() << std::endl;
        debugLog << "Exception during conversion: " << e.what() << std::endl;
        debugLog.close();
        return false;
    }
}