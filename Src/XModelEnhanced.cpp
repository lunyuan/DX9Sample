#include "XModelEnhanced.h"
#include "AllocateHierarchy.h"
#include "Utilities.h"
#include "SkinMeshFactory.h"
#include "XFileTypes.h"
#include <DirectXMath.h>
#include <iostream>

// Helper structure for frame hierarchy traversal
struct FrameInfo {
    D3DXFRAME* frame;
    D3DXMATRIX combinedTransform;
    std::string parentName;
};

std::map<std::string, std::shared_ptr<ModelData>> XModelEnhanced::LoadWithSeparation(
    const std::filesystem::path& file,
    IDirect3DDevice9* device) {
    
    if (!device) {
        throw std::invalid_argument("Device is null");
    }
    
    // Load the X file hierarchy
    AllocateHierarchy alloc(device);
    ID3DXAnimationController* animController = nullptr;
    D3DXFRAME* rootFrame = nullptr;
    
    HRESULT hr = D3DXLoadMeshHierarchyFromX(
        file.wstring().c_str(),
        D3DXMESH_MANAGED,
        device,
        &alloc,
        nullptr,
        &rootFrame,
        &animController
    );
    
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to load X file");
    }
    
    // Collect all meshes from the hierarchy
    std::vector<MeshInfo> meshes;
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    CollectMeshes(rootFrame, meshes, identity, "");
    
    // Build skeleton from frame hierarchy
    Skeleton skeleton;
    ExtractSkeleton(rootFrame, skeleton);
    
    // Convert each mesh to a separate ModelData
    std::map<std::string, std::shared_ptr<ModelData>> result;
    
    std::cout << "XModelEnhanced: Found " << meshes.size() << " meshes in X file" << std::endl;
    
    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& meshInfo = meshes[i];
        
        // Generate unique name if needed
        std::string modelName = meshInfo.name;
        if (modelName.empty()) {
            modelName = GenerateUniqueName("Object", meshes);
        }
        
        // Create ModelData for this mesh
        auto modelData = std::make_shared<ModelData>();
        
        // Convert mesh to SkinMesh format
        ConvertToSkinMesh(meshInfo, modelData->mesh, device);
        
        // Keep textures - don't clear them
        // The useOriginalTextures flag should control whether to override with SetTexture later,
        // not whether to keep the loaded textures
        char debugMsg[256];
        sprintf_s(debugMsg, "XModelEnhanced: Keeping loaded textures for %s (materials count: %zu)\n", 
                  modelName.c_str(), modelData->mesh.materials.size());
        OutputDebugStringA(debugMsg);
        
        // Assign skeleton (shared among all meshes from the same file)
        modelData->skeleton = skeleton;
        
        // Share animation controller if available
        if (animController) {
            modelData->animController = std::shared_ptr<ID3DXAnimationController>(
                animController, 
                [](ID3DXAnimationController* p) { 
                    // Don't release - shared among all models
                }
            );
        }
        
        result[modelName] = modelData;
        
        std::cout << "Extracted model: " << modelName 
                  << " (vertices: " << modelData->mesh.vertices.size() 
                  << ", triangles: " << modelData->mesh.indices.size() / 3 << ")" << std::endl;
    }
    
    // Clean up
    alloc.DestroyFrame(rootFrame);
    if (animController) {
        animController->Release();
    }
    
    return result;
}

std::vector<std::string> XModelEnhanced::GetObjectNames(
    const std::filesystem::path& file,
    IDirect3DDevice9* device) {
    
    std::vector<std::string> names;
    
    try {
        // Load the file to extract names
        AllocateHierarchy alloc(device);
        ID3DXAnimationController* animController = nullptr;
        D3DXFRAME* rootFrame = nullptr;
        
        HRESULT hr = D3DXLoadMeshHierarchyFromX(
            file.wstring().c_str(),
            D3DXMESH_MANAGED,
            device,
            &alloc,
            nullptr,
            &rootFrame,
            &animController
        );
        
        if (SUCCEEDED(hr)) {
            // Collect all meshes
            std::vector<MeshInfo> meshes;
            D3DXMATRIX identity;
            D3DXMatrixIdentity(&identity);
            CollectMeshes(rootFrame, meshes, identity, "");
            
            // Extract names
            for (const auto& mesh : meshes) {
                if (!mesh.name.empty()) {
                    names.push_back(mesh.name);
                }
            }
            
            // Clean up
            alloc.DestroyFrame(rootFrame);
            if (animController) {
                animController->Release();
            }
        }
    }
    catch (...) {
        // Return empty list on error
    }
    
    return names;
}

std::shared_ptr<ModelData> XModelEnhanced::LoadObject(
    const std::filesystem::path& file,
    const std::string& objectName,
    IDirect3DDevice9* device) {
    
    // Load all objects and return the requested one
    auto models = LoadWithSeparation(file, device);
    
    auto it = models.find(objectName);
    if (it != models.end()) {
        return it->second;
    }
    
    return nullptr;
}

void XModelEnhanced::CollectMeshes(
    D3DXFRAME* frame,
    std::vector<MeshInfo>& meshes,
    const D3DXMATRIX& parentTransform,
    const std::string& parentName) {
    
    if (!frame) return;
    
    // Calculate combined transform
    D3DXMATRIX combinedTransform = frame->TransformationMatrix * parentTransform;
    
    // Check if this frame has a mesh
    if (frame->pMeshContainer) {
        std::cout << "XModelEnhanced: Found mesh in frame: " << (frame->Name ? frame->Name : "<unnamed>") << std::endl;
        
        MeshInfo info;
        info.name = frame->Name ? frame->Name : "";
        info.parentName = parentName;
        info.transform = combinedTransform;
        
        D3DXMESHCONTAINER* meshContainer = frame->pMeshContainer;
        while (meshContainer) {
            // Extract mesh data
            info.mesh = meshContainer->MeshData.pMesh;
            
            if (info.mesh) {
                std::cout << "XModelEnhanced: Mesh has " << info.mesh->GetNumVertices() 
                          << " vertices and " << info.mesh->GetNumFaces() << " faces" << std::endl;
            }
            
            // Extract materials and textures
            char debugMsg[256];
            sprintf_s(debugMsg, "XModelEnhanced: meshContainer->pMaterials=%p, NumMaterials=%d\n", 
                      meshContainer->pMaterials, meshContainer->NumMaterials);
            OutputDebugStringA(debugMsg);
            
            if (meshContainer->pMaterials && meshContainer->NumMaterials > 0) {
                info.materials.resize(meshContainer->NumMaterials);
                info.textures.resize(meshContainer->NumMaterials);
                info.textureFileNames.resize(meshContainer->NumMaterials);
                
                sprintf_s(debugMsg, "XModelEnhanced: MeshContainer has %d materials\n", meshContainer->NumMaterials);
                OutputDebugStringA(debugMsg);
                
                for (DWORD i = 0; i < meshContainer->NumMaterials; ++i) {
                    info.materials[i] = meshContainer->pMaterials[i].MatD3D;
                    
                    // 從 MeshContainerEx 取得貼圖
                    MeshContainerEx* mc = (MeshContainerEx*)meshContainer;
                    if (mc && i < mc->m_Textures.size()) {
                        info.textures[i] = mc->m_Textures[i];
                        if (info.textures[i]) {
                            info.textures[i]->AddRef(); // 增加引用計數
                            char debugMsg[256];
                            sprintf_s(debugMsg, "XModelEnhanced: Found texture for material %d (ptr: %p)\n", i, info.textures[i]);
                            OutputDebugStringA(debugMsg);
                        }
                        // 取得檔名
                        if (i < mc->m_TextureFileNames.size() && !mc->m_TextureFileNames[i].empty()) {
                            info.textureFileNames[i] = mc->m_TextureFileNames[i];
                            sprintf_s(debugMsg, "XModelEnhanced: Texture filename: %s\n", info.textureFileNames[i].c_str());
                            OutputDebugStringA(debugMsg);
                        }
                    } else {
                        info.textures[i] = nullptr;
                    }
                }
            }
            
            // Check for skin info
            info.skinInfo = meshContainer->pSkinInfo;
            
            if (info.skinInfo) {
                char debugMsg[256];
                sprintf_s(debugMsg, "XModelEnhanced: Found skin info with %d bones\n", info.skinInfo->GetNumBones());
                OutputDebugStringA(debugMsg);
            }
            
            meshes.push_back(info);
            
            meshContainer = meshContainer->pNextMeshContainer;
        }
    }
    
    // Process siblings
    if (frame->pFrameSibling) {
        CollectMeshes(frame->pFrameSibling, meshes, parentTransform, parentName);
    }
    
    // Process children
    if (frame->pFrameFirstChild) {
        std::string currentName = frame->Name ? frame->Name : "";
        CollectMeshes(frame->pFrameFirstChild, meshes, combinedTransform, currentName);
    }
}

void XModelEnhanced::ConvertToSkinMesh(
    const MeshInfo& meshInfo,
    SkinMesh& outMesh,
    IDirect3DDevice9* device) {
    
    if (!meshInfo.mesh) return;
    
    // Get mesh data
    DWORD numVertices = meshInfo.mesh->GetNumVertices();
    DWORD numFaces = meshInfo.mesh->GetNumFaces();
    DWORD fvf = meshInfo.mesh->GetFVF();
    
    // Lock vertex buffer
    void* vertexData = nullptr;
    if (FAILED(meshInfo.mesh->LockVertexBuffer(D3DLOCK_READONLY, &vertexData))) {
        return;
    }
    
    // Extract vertex data based on FVF
    DWORD vertexSize = D3DXGetFVFVertexSize(fvf);
    outMesh.vertices.resize(numVertices);
    
    std::cout << "XModelEnhanced: Converting " << numVertices << " vertices, FVF = 0x" 
              << std::hex << fvf << std::dec << ", vertex size = " << vertexSize << std::endl;
    
    for (DWORD i = 0; i < numVertices; ++i) {
        BYTE* vertex = (BYTE*)vertexData + i * vertexSize;
        
        // Position (always present)
        if (fvf & D3DFVF_XYZ) {
            D3DXVECTOR3* pos = (D3DXVECTOR3*)vertex;
            outMesh.vertices[i].pos = DirectX::XMFLOAT3(pos->x, pos->y, pos->z);
            vertex += sizeof(D3DXVECTOR3);
        }
        
        // Normal
        if (fvf & D3DFVF_NORMAL) {
            D3DXVECTOR3* normal = (D3DXVECTOR3*)vertex;
            outMesh.vertices[i].norm = DirectX::XMFLOAT3(normal->x, normal->y, normal->z);
            vertex += sizeof(D3DXVECTOR3);
        }
        
        // Set default vertex colors (white)
        outMesh.vertices[i].col = D3DCOLOR_XRGB(255, 255, 255);
        outMesh.vertices[i].spec = D3DCOLOR_XRGB(255, 255, 255);
        
        // Texture coordinates
        if (fvf & D3DFVF_TEX1) {
            D3DXVECTOR2* texCoord = (D3DXVECTOR2*)vertex;
            outMesh.vertices[i].uv = DirectX::XMFLOAT2(texCoord->x, texCoord->y);
            vertex += sizeof(D3DXVECTOR2);
            
            // 調試：輸出前幾個頂點的UV座標
            if (i < 5) {
                char debugMsg[256];
                sprintf_s(debugMsg, "Vertex %d UV: (%.3f, %.3f)\n", i, texCoord->x, texCoord->y);
                OutputDebugStringA(debugMsg);
            }
        } else {
            // 沒有紋理座標
            outMesh.vertices[i].uv = DirectX::XMFLOAT2(0.0f, 0.0f);
            if (i == 0) {
                std::cout << "警告: 模型沒有紋理座標!" << std::endl;
            }
        }
        
        // Default bone weights and indices
        outMesh.vertices[i].weights = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        memset(outMesh.vertices[i].boneIndices, 0, sizeof(outMesh.vertices[i].boneIndices));
    }
    
    meshInfo.mesh->UnlockVertexBuffer();
    
    // Process skin info if available
    if (meshInfo.skinInfo) {
        OutputDebugStringA("ConvertToSkinMesh: Processing skin info\n");
        
        DWORD numBones = meshInfo.skinInfo->GetNumBones();
        for (DWORD i = 0; i < numVertices; ++i) {
            // Get bone influences for this vertex
            DWORD numInfluences = 0;
            DWORD bones[4] = {0, 0, 0, 0};
            float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            
            // 收集所有骨骼對這個頂點的影響
            for (DWORD boneIdx = 0; boneIdx < numBones && numInfluences < 4; ++boneIdx) {
                DWORD numVerticesInfluenced = meshInfo.skinInfo->GetNumBoneInfluences(boneIdx);
                DWORD* verticesInfluenced = nullptr;
                float* vertexWeights = nullptr;
                
                meshInfo.skinInfo->GetBoneInfluence(boneIdx, verticesInfluenced, vertexWeights);
                
                // 檢查這個骨骼是否影響當前頂點
                for (DWORD j = 0; j < numVerticesInfluenced; ++j) {
                    if (verticesInfluenced[j] == i) {
                        bones[numInfluences] = boneIdx;
                        weights[numInfluences] = vertexWeights[j];
                        numInfluences++;
                        break;
                    }
                }
            }
            
            // 正規化權重
            float totalWeight = 0.0f;
            for (int j = 0; j < 4; ++j) {
                totalWeight += weights[j];
            }
            
            if (totalWeight > 0.0f) {
                for (int j = 0; j < 4; ++j) {
                    weights[j] /= totalWeight;
                }
            }
            
            // 設置頂點的骨骼權重和索引
            outMesh.vertices[i].weights = DirectX::XMFLOAT4(weights[0], weights[1], weights[2], weights[3]);
            outMesh.vertices[i].boneIndices[0] = static_cast<uint8_t>(bones[0]);
            outMesh.vertices[i].boneIndices[1] = static_cast<uint8_t>(bones[1]);
            outMesh.vertices[i].boneIndices[2] = static_cast<uint8_t>(bones[2]);
            outMesh.vertices[i].boneIndices[3] = static_cast<uint8_t>(bones[3]);
        }
        
        // 調試：輸出前幾個頂點的骨骼權重
        DWORD maxDebugVerts = (numVertices < 5) ? numVertices : 5;
        for (DWORD i = 0; i < maxDebugVerts; ++i) {
            char debugMsg[256];
            sprintf_s(debugMsg, "Vertex %d: weights(%.3f, %.3f, %.3f, %.3f) bones(%d, %d, %d, %d)\n",
                      i, 
                      outMesh.vertices[i].weights.x, outMesh.vertices[i].weights.y,
                      outMesh.vertices[i].weights.z, outMesh.vertices[i].weights.w,
                      outMesh.vertices[i].boneIndices[0], outMesh.vertices[i].boneIndices[1],
                      outMesh.vertices[i].boneIndices[2], outMesh.vertices[i].boneIndices[3]);
            OutputDebugStringA(debugMsg);
        }
    }
    
    // Lock index buffer
    void* indexData = nullptr;
    if (FAILED(meshInfo.mesh->LockIndexBuffer(D3DLOCK_READONLY, &indexData))) {
        return;
    }
    
    // Extract indices
    outMesh.indices.resize(numFaces * 3);
    
    if (meshInfo.mesh->GetOptions() & D3DXMESH_32BIT) {
        // 32-bit indices
        DWORD* indices32 = (DWORD*)indexData;
        for (DWORD i = 0; i < numFaces * 3; ++i) {
            outMesh.indices[i] = indices32[i];
        }
    } else {
        // 16-bit indices
        WORD* indices16 = (WORD*)indexData;
        for (DWORD i = 0; i < numFaces * 3; ++i) {
            outMesh.indices[i] = indices16[i];
        }
    }
    
    meshInfo.mesh->UnlockIndexBuffer();
    
    // Copy materials
    outMesh.materials.resize(meshInfo.materials.size());
    
    char debugMsg[256];
    sprintf_s(debugMsg, "ConvertToSkinMesh: Copying %zu materials\n", meshInfo.materials.size());
    OutputDebugStringA(debugMsg);
    
    for (size_t i = 0; i < meshInfo.materials.size(); ++i) {
        outMesh.materials[i].mat = meshInfo.materials[i];
        outMesh.materials[i].tex = meshInfo.textures[i];
        // 設定檔名（如果有的話）
        if (i < meshInfo.textureFileNames.size()) {
            outMesh.materials[i].textureFileName = meshInfo.textureFileNames[i];
        }
    }
    
    // Apply transformation to vertices
    for (size_t i = 0; i < outMesh.vertices.size(); ++i) {
        auto& vertex = outMesh.vertices[i];
        D3DXVECTOR3 pos(vertex.pos.x, vertex.pos.y, vertex.pos.z);
        D3DXVECTOR3 transformedPos;
        D3DXVec3TransformCoord(&transformedPos, &pos, &meshInfo.transform);
        vertex.pos = DirectX::XMFLOAT3(transformedPos.x, transformedPos.y, transformedPos.z);
        
        // 輸出前幾個頂點的位置信息
        if (i < 5) {
            std::cout << "Vertex " << i << ": (" << transformedPos.x << ", " 
                      << transformedPos.y << ", " << transformedPos.z << ")" << std::endl;
        }
        
        if (fvf & D3DFVF_NORMAL) {
            D3DXVECTOR3 normal(vertex.norm.x, vertex.norm.y, vertex.norm.z);
            D3DXVECTOR3 transformedNormal;
            D3DXVec3TransformNormal(&transformedNormal, &normal, &meshInfo.transform);
            D3DXVec3Normalize(&transformedNormal, &transformedNormal);
            vertex.norm = DirectX::XMFLOAT3(transformedNormal.x, transformedNormal.y, transformedNormal.z);
        }
    }
    
    // Create D3D buffers
    outMesh.CreateBuffers(device);
}

void XModelEnhanced::ExtractSkeleton(
    D3DXFRAME* frame,
    Skeleton& skeleton,
    int parentIndex) {
    
    if (!frame) return;
    
    // Add this frame as a joint
    int currentIndex = static_cast<int>(skeleton.joints.size());
    
    SkeletonJoint joint;
    joint.name = frame->Name ? frame->Name : "Joint_" + std::to_string(currentIndex);
    joint.parentIndex = parentIndex;
    
    // Convert D3DXMATRIX to DirectX::XMFLOAT4X4
    DirectX::XMFLOAT4X4 transform;
    memcpy(&transform, &frame->TransformationMatrix, sizeof(DirectX::XMFLOAT4X4));
    joint.bindPoseInverse = transform;
    
    skeleton.joints.push_back(joint);
    
    // Process children
    if (frame->pFrameFirstChild) {
        ExtractSkeleton(frame->pFrameFirstChild, skeleton, currentIndex);
    }
    
    // Process siblings
    if (frame->pFrameSibling) {
        ExtractSkeleton(frame->pFrameSibling, skeleton, parentIndex);
    }
}

std::string XModelEnhanced::GenerateUniqueName(
    const std::string& baseName,
    const std::vector<MeshInfo>& existingMeshes) {
    
    std::string uniqueName = baseName;
    int counter = 1;
    
    // Check if name already exists
    auto nameExists = [&](const std::string& name) {
        for (const auto& mesh : existingMeshes) {
            if (mesh.name == name) {
                return true;
            }
        }
        return false;
    };
    
    while (nameExists(uniqueName)) {
        uniqueName = baseName + "_" + std::to_string(counter++);
    }
    
    return uniqueName;
}