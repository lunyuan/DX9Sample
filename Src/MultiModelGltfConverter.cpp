#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>
#include <d3d9.h>
#include <d3dx9.h>
#include "IAssetManager.h"
#include "ModelData.h"
#include "tiny_gltf.h"
#include <algorithm>
#include <cstdio>  // for sprintf_s and OutputDebugStringA
#include <windows.h>

// Prevent min/max macro conflicts
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// 多模型 X to glTF 轉換函式
bool ConvertXToGltfMultiModel(IDirect3DDevice9* device, IAssetManager* assetManager, 
                              const std::string& xFile, const std::string& gltfFile) {
    
    try {
        // 檢查檔案是否存在
        if (!std::filesystem::exists(xFile)) {
            return false;
        }
        
        // 載入所有模型
        auto models = assetManager->LoadAllModels(xFile);
        if (models.empty()) {
            return false;
        }
        
        
        // 創建 glTF 模型
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltf;
        
        // 設置資產信息
        gltfModel.asset.version = "2.0";
        gltfModel.asset.generator = "DX9Sample MultiModelGltfConverter";
        
        // 創建場景
        tinygltf::Scene scene;
        scene.name = "Scene";
        
        // 使用單一緩衝區存放所有模型資料
        std::vector<uint8_t> bufferData;
        
        // 為每個模型創建網格
        for (size_t modelIdx = 0; modelIdx < models.size(); ++modelIdx) {
            const auto& xModel = *models[modelIdx];
            
            // 準備頂點資料
            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<float> texcoords;
            
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
            
            // 記錄資料在緩衝區中的位置
            size_t posOffset = bufferData.size();
            size_t posSize = positions.size() * sizeof(float);
            bufferData.resize(bufferData.size() + posSize);
            memcpy(bufferData.data() + posOffset, positions.data(), posSize);
            
            size_t normOffset = bufferData.size();
            size_t normSize = normals.size() * sizeof(float);
            bufferData.resize(bufferData.size() + normSize);
            memcpy(bufferData.data() + normOffset, normals.data(), normSize);
            
            size_t uvOffset = bufferData.size();
            size_t uvSize = texcoords.size() * sizeof(float);
            bufferData.resize(bufferData.size() + uvSize);
            memcpy(bufferData.data() + uvOffset, texcoords.data(), uvSize);
            
            size_t indexOffset = bufferData.size();
            size_t indexSize = xModel.mesh.indices.size() * sizeof(uint32_t);
            bufferData.resize(bufferData.size() + indexSize);
            memcpy(bufferData.data() + indexOffset, xModel.mesh.indices.data(), indexSize);
            
            // 創建緩衝視圖
            size_t baseViewIdx = gltfModel.bufferViews.size();
            
            // 位置視圖
            tinygltf::BufferView posView;
            posView.buffer = 0;
            posView.byteOffset = posOffset;
            posView.byteLength = posSize;
            posView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(posView);
            
            // 法線視圖
            tinygltf::BufferView normView;
            normView.buffer = 0;
            normView.byteOffset = normOffset;
            normView.byteLength = normSize;
            normView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(normView);
            
            // UV視圖
            tinygltf::BufferView uvView;
            uvView.buffer = 0;
            uvView.byteOffset = uvOffset;
            uvView.byteLength = uvSize;
            uvView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(uvView);
            
            // 索引視圖
            tinygltf::BufferView indexView;
            indexView.buffer = 0;
            indexView.byteOffset = indexOffset;
            indexView.byteLength = indexSize;
            indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(indexView);
            
            // 創建存取器
            size_t baseAccessorIdx = gltfModel.accessors.size();
            
            // 位置存取器
            tinygltf::Accessor posAccessor;
            posAccessor.bufferView = static_cast<int>(baseViewIdx);
            posAccessor.byteOffset = 0;
            posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            posAccessor.count = positions.size() / 3;
            posAccessor.type = TINYGLTF_TYPE_VEC3;
            
            // 計算邊界
            std::vector<double> posMin(3, std::numeric_limits<double>::max());
            std::vector<double> posMax(3, std::numeric_limits<double>::lowest());
            for (size_t i = 0; i < positions.size(); i += 3) {
                posMin[0] = std::min(posMin[0], (double)positions[i]);
                posMin[1] = std::min(posMin[1], (double)positions[i + 1]);
                posMin[2] = std::min(posMin[2], (double)positions[i + 2]);
                posMax[0] = std::max(posMax[0], (double)positions[i]);
                posMax[1] = std::max(posMax[1], (double)positions[i + 1]);
                posMax[2] = std::max(posMax[2], (double)positions[i + 2]);
            }
            posAccessor.minValues = posMin;
            posAccessor.maxValues = posMax;
            gltfModel.accessors.push_back(posAccessor);
            
            // 法線存取器
            tinygltf::Accessor normAccessor;
            normAccessor.bufferView = static_cast<int>(baseViewIdx + 1);
            normAccessor.byteOffset = 0;
            normAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            normAccessor.count = normals.size() / 3;
            normAccessor.type = TINYGLTF_TYPE_VEC3;
            gltfModel.accessors.push_back(normAccessor);
            
            // UV存取器
            tinygltf::Accessor uvAccessor;
            uvAccessor.bufferView = static_cast<int>(baseViewIdx + 2);
            uvAccessor.byteOffset = 0;
            uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            uvAccessor.count = texcoords.size() / 2;
            uvAccessor.type = TINYGLTF_TYPE_VEC2;
            gltfModel.accessors.push_back(uvAccessor);
            
            // 索引存取器
            tinygltf::Accessor indexAccessor;
            indexAccessor.bufferView = static_cast<int>(baseViewIdx + 3);
            indexAccessor.byteOffset = 0;
            indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
            indexAccessor.count = xModel.mesh.indices.size();
            indexAccessor.type = TINYGLTF_TYPE_SCALAR;
            gltfModel.accessors.push_back(indexAccessor);
            
            // 創建圖元
            tinygltf::Primitive primitive;
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            primitive.attributes["POSITION"] = static_cast<int>(baseAccessorIdx);
            primitive.attributes["NORMAL"] = static_cast<int>(baseAccessorIdx + 1);
            primitive.attributes["TEXCOORD_0"] = static_cast<int>(baseAccessorIdx + 2);
            primitive.indices = static_cast<int>(baseAccessorIdx + 3);
            
            // 處理材質和貼圖
            if (!xModel.mesh.materials.empty()) {
                // 為每個材質創建對應的 glTF 材質
                size_t materialStartIdx = gltfModel.materials.size();
                
                for (size_t matIdx = 0; matIdx < xModel.mesh.materials.size(); ++matIdx) {
                    const auto& xMat = xModel.mesh.materials[matIdx];
                    
                    tinygltf::Material material;
                    material.name = "Material_" + std::to_string(modelIdx) + "_" + std::to_string(matIdx);
                    
                    // 設置基本顏色
                    material.pbrMetallicRoughness.baseColorFactor = {
                        xMat.mat.Diffuse.r,
                        xMat.mat.Diffuse.g,
                        xMat.mat.Diffuse.b,
                        xMat.mat.Diffuse.a
                    };
                    
                    material.pbrMetallicRoughness.metallicFactor = 0.0;
                    material.pbrMetallicRoughness.roughnessFactor = 0.5;
                    material.doubleSided = true;
                    
                    // 處理貼圖
                    if (!xMat.textureFileName.empty()) {
                        // 檢查貼圖是否已經存在
                        int imageIdx = -1;
                        for (size_t i = 0; i < gltfModel.images.size(); ++i) {
                            if (gltfModel.images[i].uri == xMat.textureFileName) {
                                imageIdx = static_cast<int>(i);
                                break;
                            }
                        }
                        
                        // 如果貼圖不存在，創建新的
                        if (imageIdx == -1) {
                            tinygltf::Image image;
                            image.uri = xMat.textureFileName;
                            imageIdx = static_cast<int>(gltfModel.images.size());
                            gltfModel.images.push_back(image);
                            
                            // 創建貼圖
                            tinygltf::Texture texture;
                            texture.source = imageIdx;
                            int textureIdx = static_cast<int>(gltfModel.textures.size());
                            gltfModel.textures.push_back(texture);
                            
                            // 設置材質的基本顏色貼圖
                            material.pbrMetallicRoughness.baseColorTexture.index = textureIdx;
                            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
                        } else {
                            // 使用現有的貼圖
                            material.pbrMetallicRoughness.baseColorTexture.index = imageIdx;
                            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
                        }
                        
                        char debugMsg[256];
                        sprintf_s(debugMsg, "glTF Converter: Added texture '%s' for model %zu material %zu\n",
                                  xMat.textureFileName.c_str(), modelIdx, matIdx);
                        OutputDebugStringA(debugMsg);
                    }
                    
                    gltfModel.materials.push_back(material);
                }
                
                // 使用第一個材質（簡化處理）
                primitive.material = static_cast<int>(materialStartIdx);
            } else {
                // 如果沒有材質，創建預設材質
                if (gltfModel.materials.empty()) {
                    tinygltf::Material material;
                    material.name = "DefaultMaterial";
                    material.pbrMetallicRoughness.baseColorFactor = {1.0, 1.0, 1.0, 1.0};
                    material.pbrMetallicRoughness.metallicFactor = 0.0;
                    material.pbrMetallicRoughness.roughnessFactor = 0.5;
                    material.doubleSided = true;
                    gltfModel.materials.push_back(material);
                }
                primitive.material = 0;
            }
            
            // 創建網格
            tinygltf::Mesh mesh;
            mesh.name = "Mesh_" + std::to_string(modelIdx);
            mesh.primitives.push_back(primitive);
            gltfModel.meshes.push_back(mesh);
            
            // 創建節點
            tinygltf::Node node;
            node.name = "Model_" + std::to_string(modelIdx);
            node.mesh = static_cast<int>(modelIdx);
            gltfModel.nodes.push_back(node);
            
            // 添加到場景
            scene.nodes.push_back(static_cast<int>(modelIdx));
        }
        
        // 創建緩衝區
        tinygltf::Buffer buffer;
        buffer.data = bufferData;
        gltfModel.buffers.push_back(buffer);
        
        // 設置場景
        gltfModel.scenes.push_back(scene);
        gltfModel.defaultScene = 0;
        
        // 寫入檔案
        bool ret = gltf.WriteGltfSceneToFile(&gltfModel, gltfFile, 
                                            true,  // embedImages
                                            true,  // embedBuffers
                                            true,  // prettyPrint
                                            false); // writeBinary
        
        return ret;
        
    } catch (const std::exception&) {
        return false;
    }
}