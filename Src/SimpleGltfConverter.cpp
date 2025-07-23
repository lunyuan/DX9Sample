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

// 簡單的 X to glTF 轉換函式
bool SimpleConvertXToGltf(IDirect3DDevice9* device, const std::string& xFile, const std::string& gltfFile) {
    
    // 創建 AssetManager
    auto assetManager = CreateAssetManager();
    assetManager->Initialize(device);
    
    // 載入 X 檔案
    try {
        
        // 檢查檔案是否存在
        if (!std::filesystem::exists(xFile)) {
            return false;
        }
        
        auto models = assetManager->LoadAllModels(xFile);
        if (models.empty()) {
            return false;
        }
        
        // 使用第一個模型
        auto& xModel = *models[0];
        
        
        // 創建 glTF 模型
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltf;
        
        // 設置資產信息
        gltfModel.asset.version = "2.0";
        gltfModel.asset.generator = "DX9Sample SimpleConverter";
        
        // 創建場景
        tinygltf::Scene scene;
        scene.name = "Scene";
        
        // 創建節點
        tinygltf::Node node;
        node.name = "Model";
        
        // 創建網格
        tinygltf::Mesh gltfMesh;
        gltfMesh.name = "Mesh";
        
        // 創建原始圖形
        tinygltf::Primitive primitive;
        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        
        // 轉換頂點數據
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> texcoords;
        
        for (const auto& v : xModel.mesh.vertices) {
            positions.push_back(v.pos.x);
            positions.push_back(v.pos.y);
            positions.push_back(v.pos.z);
            
            normals.push_back(v.norm.x);
            normals.push_back(v.norm.y);
            normals.push_back(v.norm.z);
            
            texcoords.push_back(v.uv.x);
            texcoords.push_back(1.0f - v.uv.y); // Flip Y for glTF
        }
        
        // 創建緩衝區
        tinygltf::Buffer buffer;
        size_t totalSize = positions.size() * sizeof(float) + 
                          normals.size() * sizeof(float) + 
                          texcoords.size() * sizeof(float) +
                          xModel.mesh.indices.size() * sizeof(uint32_t);
        buffer.data.resize(totalSize);
        
        size_t offset = 0;
        
        // 複製位置數據
        memcpy(buffer.data.data() + offset, positions.data(), positions.size() * sizeof(float));
        size_t posOffset = offset;
        offset += positions.size() * sizeof(float);
        
        // 複製法線數據
        memcpy(buffer.data.data() + offset, normals.data(), normals.size() * sizeof(float));
        size_t normOffset = offset;
        offset += normals.size() * sizeof(float);
        
        // 複製紋理坐標數據
        memcpy(buffer.data.data() + offset, texcoords.data(), texcoords.size() * sizeof(float));
        size_t uvOffset = offset;
        offset += texcoords.size() * sizeof(float);
        
        // 複製索引數據
        memcpy(buffer.data.data() + offset, xModel.mesh.indices.data(), xModel.mesh.indices.size() * sizeof(uint32_t));
        size_t indexOffset = offset;
        
        gltfModel.buffers.push_back(buffer);
        
        // 創建緩衝視圖
        tinygltf::BufferView posView;
        posView.buffer = 0;
        posView.byteOffset = posOffset;
        posView.byteLength = positions.size() * sizeof(float);
        posView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        gltfModel.bufferViews.push_back(posView);
        
        tinygltf::BufferView normView;
        normView.buffer = 0;
        normView.byteOffset = normOffset;
        normView.byteLength = normals.size() * sizeof(float);
        normView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        gltfModel.bufferViews.push_back(normView);
        
        tinygltf::BufferView uvView;
        uvView.buffer = 0;
        uvView.byteOffset = uvOffset;
        uvView.byteLength = texcoords.size() * sizeof(float);
        uvView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        gltfModel.bufferViews.push_back(uvView);
        
        tinygltf::BufferView indexView;
        indexView.buffer = 0;
        indexView.byteOffset = indexOffset;
        indexView.byteLength = xModel.mesh.indices.size() * sizeof(uint32_t);
        indexView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        gltfModel.bufferViews.push_back(indexView);
        
        // 創建存取器
        tinygltf::Accessor posAccessor;
        posAccessor.bufferView = 0;
        posAccessor.byteOffset = 0;
        posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        posAccessor.count = xModel.mesh.vertices.size();
        posAccessor.type = TINYGLTF_TYPE_VEC3;
        
        // 計算邊界
        std::vector<double> posMin(3, std::numeric_limits<double>::max());
        std::vector<double> posMax(3, std::numeric_limits<double>::lowest());
        for (size_t i = 0; i < positions.size(); i += 3) {
            for (int j = 0; j < 3; ++j) {
                posMin[j] = std::min(posMin[j], static_cast<double>(positions[i + j]));
                posMax[j] = std::max(posMax[j], static_cast<double>(positions[i + j]));
            }
        }
        posAccessor.minValues = posMin;
        posAccessor.maxValues = posMax;
        gltfModel.accessors.push_back(posAccessor);
        
        tinygltf::Accessor normAccessor;
        normAccessor.bufferView = 1;
        normAccessor.byteOffset = 0;
        normAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        normAccessor.count = xModel.mesh.vertices.size();
        normAccessor.type = TINYGLTF_TYPE_VEC3;
        gltfModel.accessors.push_back(normAccessor);
        
        tinygltf::Accessor uvAccessor;
        uvAccessor.bufferView = 2;
        uvAccessor.byteOffset = 0;
        uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        uvAccessor.count = xModel.mesh.vertices.size();
        uvAccessor.type = TINYGLTF_TYPE_VEC2;
        gltfModel.accessors.push_back(uvAccessor);
        
        tinygltf::Accessor indexAccessor;
        indexAccessor.bufferView = 3;
        indexAccessor.byteOffset = 0;
        indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        indexAccessor.count = xModel.mesh.indices.size();
        indexAccessor.type = TINYGLTF_TYPE_SCALAR;
        gltfModel.accessors.push_back(indexAccessor);
        
        // 設置原始圖形屬性
        primitive.attributes["POSITION"] = 0;
        primitive.attributes["NORMAL"] = 1;
        primitive.attributes["TEXCOORD_0"] = 2;
        primitive.indices = 3;
        
        // 創建簡單材質
        tinygltf::Material material;
        material.name = "Material";
        material.pbrMetallicRoughness.baseColorFactor = {1.0, 1.0, 1.0, 1.0};
        material.pbrMetallicRoughness.metallicFactor = 0.0;
        material.pbrMetallicRoughness.roughnessFactor = 0.5;
        gltfModel.materials.push_back(material);
        primitive.material = 0;
        
        gltfMesh.primitives.push_back(primitive);
        gltfModel.meshes.push_back(gltfMesh);
        
        node.mesh = 0;
        gltfModel.nodes.push_back(node);
        
        scene.nodes.push_back(0);
        gltfModel.scenes.push_back(scene);
        gltfModel.defaultScene = 0;
        
        // 保存文件
        bool embedImages = true;
        bool embedBuffers = true;
        bool prettyPrint = true;
        bool writeBinary = false;
        
        if (gltf.WriteGltfSceneToFile(&gltfModel, gltfFile,
            embedImages, embedBuffers, prettyPrint, writeBinary)) {
            return true;
        } else {
            return false;
        }
        
    } catch (const std::exception& e) {
        return false;
    }
}

// 使用現有 AssetManager 的轉換函式
bool SimpleConvertXToGltfWithAssetManager(IDirect3DDevice9* device, IAssetManager* assetManager, 
                                         const std::string& xFile, const std::string& gltfFile) {
    
    // 載入 X 檔案
    try {
        
        // 檢查檔案是否存在
        if (!std::filesystem::exists(xFile)) {
            return false;
        }
        
        auto models = assetManager->LoadAllModels(xFile);
        if (models.empty()) {
            return false;
        }
        
        // 計算所有模型的總頂點和索引數
        size_t totalVertices = 0;
        size_t totalIndices = 0;
        for (const auto& model : models) {
            totalVertices += model->mesh.vertices.size();
            totalIndices += model->mesh.indices.size();
        }
        
        
        // 創建 glTF 模型
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltf;
        
        // 設置資產信息
        gltfModel.asset.version = "2.0";
        gltfModel.asset.generator = "DX9Sample SimpleGltfConverter";
        
        // 創建場景
        tinygltf::Scene scene;
        scene.name = "Scene";
        
        
        // 使用第一個模型
        const auto& xModel = *models[0];
        
        
        // 創建節點
        tinygltf::Node node;
        node.name = "Model";
        
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
        std::vector<unsigned int> indices;
        
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
        
        // 轉換索引數據
        indices = xModel.mesh.indices;
        
        // 創建緩衝區
        // 位置緩衝區
        {
            tinygltf::Buffer buffer;
            buffer.data.resize(positions.size() * sizeof(float));
            memcpy(buffer.data.data(), positions.data(), buffer.data.size());
            gltfModel.buffers.push_back(buffer);
            
            tinygltf::BufferView bufferView;
            bufferView.buffer = 0;
            bufferView.byteOffset = 0;
            bufferView.byteLength = buffer.data.size();
            bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(bufferView);
            
            tinygltf::Accessor accessor;
            accessor.bufferView = 0;
            accessor.byteOffset = 0;
            accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            accessor.count = positions.size() / 3;
            accessor.type = TINYGLTF_TYPE_VEC3;
            
            // 計算邊界
            for (size_t i = 0; i < positions.size() / 3; ++i) {
                float x = positions[i * 3];
                float y = positions[i * 3 + 1];
                float z = positions[i * 3 + 2];
                
                if (i == 0) {
                    accessor.minValues = {x, y, z};
                    accessor.maxValues = {x, y, z};
                } else {
                    accessor.minValues[0] = std::min(accessor.minValues[0], (double)x);
                    accessor.minValues[1] = std::min(accessor.minValues[1], (double)y);
                    accessor.minValues[2] = std::min(accessor.minValues[2], (double)z);
                    accessor.maxValues[0] = std::max(accessor.maxValues[0], (double)x);
                    accessor.maxValues[1] = std::max(accessor.maxValues[1], (double)y);
                    accessor.maxValues[2] = std::max(accessor.maxValues[2], (double)z);
                }
            }
            
            gltfModel.accessors.push_back(accessor);
            primitive.attributes["POSITION"] = 0;
        }
        
        // 法線緩衝區
        {
            tinygltf::Buffer buffer;
            buffer.data.resize(normals.size() * sizeof(float));
            memcpy(buffer.data.data(), normals.data(), buffer.data.size());
            gltfModel.buffers.push_back(buffer);
            
            tinygltf::BufferView bufferView;
            bufferView.buffer = 1;
            bufferView.byteOffset = 0;
            bufferView.byteLength = buffer.data.size();
            bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(bufferView);
            
            tinygltf::Accessor accessor;
            accessor.bufferView = 1;
            accessor.byteOffset = 0;
            accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            accessor.count = normals.size() / 3;
            accessor.type = TINYGLTF_TYPE_VEC3;
            gltfModel.accessors.push_back(accessor);
            primitive.attributes["NORMAL"] = 1;
        }
        
        // 紋理座標緩衝區
        {
            tinygltf::Buffer buffer;
            buffer.data.resize(texcoords.size() * sizeof(float));
            memcpy(buffer.data.data(), texcoords.data(), buffer.data.size());
            gltfModel.buffers.push_back(buffer);
            
            tinygltf::BufferView bufferView;
            bufferView.buffer = 2;
            bufferView.byteOffset = 0;
            bufferView.byteLength = buffer.data.size();
            bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(bufferView);
            
            tinygltf::Accessor accessor;
            accessor.bufferView = 2;
            accessor.byteOffset = 0;
            accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            accessor.count = texcoords.size() / 2;
            accessor.type = TINYGLTF_TYPE_VEC2;
            gltfModel.accessors.push_back(accessor);
            primitive.attributes["TEXCOORD_0"] = 2;
        }
        
        // 索引緩衝區
        {
            tinygltf::Buffer buffer;
            buffer.data.resize(indices.size() * sizeof(unsigned int));
            memcpy(buffer.data.data(), indices.data(), buffer.data.size());
            gltfModel.buffers.push_back(buffer);
            
            tinygltf::BufferView bufferView;
            bufferView.buffer = 3;
            bufferView.byteOffset = 0;
            bufferView.byteLength = buffer.data.size();
            bufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            gltfModel.bufferViews.push_back(bufferView);
            
            tinygltf::Accessor accessor;
            accessor.bufferView = 3;
            accessor.byteOffset = 0;
            accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
            accessor.count = indices.size();
            accessor.type = TINYGLTF_TYPE_SCALAR;
            gltfModel.accessors.push_back(accessor);
            primitive.indices = 3;
        }
        
        // 創建材質
        tinygltf::Material material;
        material.name = "Material";
        material.pbrMetallicRoughness.baseColorFactor = {1.0, 1.0, 1.0, 1.0};
        material.pbrMetallicRoughness.metallicFactor = 0.0;
        material.pbrMetallicRoughness.roughnessFactor = 0.5;
        material.doubleSided = true;
        gltfModel.materials.push_back(material);
        primitive.material = 0;
        
        // 組裝模型
        mesh.primitives.push_back(primitive);
        gltfModel.meshes.push_back(mesh);
        
        node.mesh = 0;
        scene.nodes.push_back(0);
        gltfModel.nodes.push_back(node);
        gltfModel.scenes.push_back(scene);
        gltfModel.defaultScene = 0;
        
        // 寫入檔案
        bool ret = gltf.WriteGltfSceneToFile(&gltfModel, gltfFile, 
                                            true, // embedImages
                                            true, // embedBuffers
                                            true, // prettyPrint
                                            false); // writeBinary (false for .gltf, true for .glb)
        
        if (ret) {
            return true;
        } else {
            return false;
        }
        
    } catch (const std::exception& e) {
        return false;
    }
}

