#include "FbxLoader.h"
#include <iostream>
#include <iostream>
#include <functional>
#include <algorithm>
#include <DirectXMath.h>
#include <d3dx9.h>
#include "AnimationPlayer.h"
#include "SkinMeshFactory.h"

// Need access to InitVertexDecl
void InitVertexDecl(IDirect3DDevice9* dev);

using namespace fbxsdk;

FbxLoader::FbxLoader() {
}

FbxLoader::~FbxLoader() {
}

std::map<std::string, ModelData> FbxLoader::Load(const std::filesystem::path& file, IDirect3DDevice9* device) const {
    std::map<std::string, ModelData> result;
    
    // Create FBX Manager and Scene
    FbxManager* mgr = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(mgr, IOSROOT);
    mgr->SetIOSettings(ios);
    
    FbxScene* scene = FbxScene::Create(mgr, "scene");
    
    // Load the scene
    if (!LoadScene(file.string(), mgr, scene)) {
        scene->Destroy();
        mgr->Destroy();
        return result;
    }
    
    // Convert to global transformation
    FbxGeometryConverter converter(mgr);
    converter.Triangulate(scene, true);
    
    // Convert coordinate system if needed (FBX uses Y-up, right-handed by default)
    // DirectX uses left-handed coordinate system
    FbxAxisSystem directXAxisSystem(FbxAxisSystem::eDirectX);
    directXAxisSystem.ConvertScene(scene);
    
    // Process root node
    FbxNode* root = scene->GetRootNode();
    if (!root) {
        scene->Destroy();
        mgr->Destroy();
        return result;
    }
    
    // Create model data
    ModelData modelData;
    
    // Extract skeleton first
    ExtractSkeleton(root, modelData.skeleton);
    
    // Extract animations
    ExtractAnimations(scene, modelData.skeleton);
    
    // Convert nodes to mesh
    ConvertNode(root, modelData.mesh, modelData.skeleton, device);
    
    // Create buffers after all mesh data is collected
    if (!modelData.mesh.vertices.empty()) {
        if (!modelData.mesh.CreateBuffers(device)) {
            std::cerr << "FBX: Failed to create vertex/index buffers" << std::endl;
        }
    }
    
    // Note: Animation controller would be created here if needed
    // modelData.animController = ...;
    
    // Add to result with the base filename as key
    std::string modelName = file.stem().string();
    result[modelName] = std::move(modelData);
    
    // Cleanup
    scene->Destroy();
    mgr->Destroy();
    
    return result;
}

std::vector<std::string> FbxLoader::GetModelNames(const std::filesystem::path& file) const {
    std::vector<std::string> names;
    
    // For FBX files, we typically have one model per file
    // Return the filename without extension as the model name
    names.push_back(file.stem().string());
    
    return names;
}

bool FbxLoader::LoadScene(const std::string& path, FbxManager* mgr, FbxScene* scene) const {
    FbxImporter* importer = FbxImporter::Create(mgr, "");
    
    if (!importer->Initialize(path.c_str(), -1, mgr->GetIOSettings())) {
        std::cerr << "Failed to initialize FBX importer for " << path << std::endl;
        std::cerr << "Error: " << importer->GetStatus().GetErrorString() << std::endl;
        importer->Destroy();
        return false;
    }
    
    if (!importer->Import(scene)) {
        std::cerr << "Failed to import FBX scene from " << path << std::endl;
        importer->Destroy();
        return false;
    }
    
    importer->Destroy();
    return true;
}

void FbxLoader::ConvertNode(FbxNode* node, SkinMesh& mesh, Skeleton& skel, IDirect3DDevice9* device) const {
    if (!node) return;
    
    // Check if this node has a mesh
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
        FbxMesh* fbxMesh = node->GetMesh();
        if (fbxMesh) {
            ExtractMeshData(fbxMesh, mesh, device);
            ExtractMaterials(node, mesh, device);
        }
    }
    
    // Process children
    for (int i = 0; i < node->GetChildCount(); ++i) {
        ConvertNode(node->GetChild(i), mesh, skel, device);
    }
}

void FbxLoader::ExtractMeshData(FbxMesh* fbxMesh, SkinMesh& mesh, IDirect3DDevice9* device) const {
    if (!fbxMesh) return;
    
    // Initialize vertex declaration if needed
    InitVertexDecl(device);
    
    int vertexCount = fbxMesh->GetControlPointsCount();
    int polyCount = fbxMesh->GetPolygonCount();
    
    // Extract skin deformer data
    std::vector<std::vector<std::pair<int, float>>> skinWeights(vertexCount);
    ExtractSkinWeights(fbxMesh, skinWeights);
    
    // Get control points (vertex positions)
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();
    
    // Get layers for normals and UVs
    FbxLayer* layer0 = fbxMesh->GetLayer(0);
    FbxLayerElementNormal* normalElement = layer0 ? layer0->GetNormals() : nullptr;
    FbxLayerElementUV* uvElement = layer0 ? layer0->GetUVs() : nullptr;
    
    // Get current vertex count to offset indices
    uint32_t baseVertexIndex = static_cast<uint32_t>(mesh.vertices.size());
    
    // Reserve space for vertices (append to existing)
    std::vector<Vertex> vertices;
    vertices.reserve(polyCount * 3); // Assuming triangles
    
    // Extract vertex data
    std::vector<uint32_t> indices;
    int vertexCounter = 0;
    
    for (int polyIdx = 0; polyIdx < polyCount; ++polyIdx) {
        int polySize = fbxMesh->GetPolygonSize(polyIdx);
        
        // Store vertices for this polygon first
        std::vector<uint32_t> polyVertices;
        
        for (int vertIdx = 0; vertIdx < polySize; ++vertIdx) {
            Vertex vertex = {};
            
            // Get control point index
            int cpIndex = fbxMesh->GetPolygonVertex(polyIdx, vertIdx);
            
            // Position
            FbxVector4 pos = controlPoints[cpIndex];
            vertex.pos.x = static_cast<float>(pos[0]);
            vertex.pos.y = static_cast<float>(pos[1]);
            vertex.pos.z = static_cast<float>(pos[2]);
            
            // Normal
            if (normalElement) {
                FbxVector4 normal;
                int vertexIndex = polyIdx * 3 + vertIdx;
                
                switch (normalElement->GetMappingMode()) {
                    case FbxLayerElement::eByControlPoint:
                        switch (normalElement->GetReferenceMode()) {
                            case FbxLayerElement::eDirect:
                                normal = normalElement->GetDirectArray().GetAt(cpIndex);
                                break;
                            case FbxLayerElement::eIndexToDirect:
                                {
                                    int index = normalElement->GetIndexArray().GetAt(cpIndex);
                                    normal = normalElement->GetDirectArray().GetAt(index);
                                }
                                break;
                        }
                        break;
                    case FbxLayerElement::eByPolygonVertex:
                        switch (normalElement->GetReferenceMode()) {
                            case FbxLayerElement::eDirect:
                                normal = normalElement->GetDirectArray().GetAt(vertexIndex);
                                break;
                            case FbxLayerElement::eIndexToDirect:
                                {
                                    int index = normalElement->GetIndexArray().GetAt(vertexIndex);
                                    normal = normalElement->GetDirectArray().GetAt(index);
                                }
                                break;
                        }
                        break;
                }
                
                vertex.norm.x = static_cast<float>(normal[0]);
                vertex.norm.y = static_cast<float>(normal[1]);
                vertex.norm.z = static_cast<float>(normal[2]);
            }
            
            // UV
            if (uvElement) {
                FbxVector2 uv;
                bool unmapped;
                fbxMesh->GetPolygonVertexUV(polyIdx, vertIdx, uvElement->GetName(), uv, unmapped);
                
                if (!unmapped) {
                    vertex.uv.x = static_cast<float>(uv[0]);
                    vertex.uv.y = 1.0f - static_cast<float>(uv[1]); // Flip V coordinate
                }
            }
            
            // Default color
            vertex.col = 0xFFFFFFFF;
            vertex.spec = 0xFFFFFFFF;
            
            // Skinning weights - extract from skin data
            if (cpIndex < skinWeights.size() && !skinWeights[cpIndex].empty()) {
                // Sort weights by influence (highest first)
                auto weights = skinWeights[cpIndex];
                std::sort(weights.begin(), weights.end(), 
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                // Take up to 4 bone influences
                float totalWeight = 0.0f;
                for (int w = 0; w < 4 && w < weights.size(); ++w) {
                    switch (w) {
                        case 0: vertex.weights.x = weights[w].second; break;
                        case 1: vertex.weights.y = weights[w].second; break;
                        case 2: vertex.weights.z = weights[w].second; break;
                        case 3: vertex.weights.w = weights[w].second; break;
                    }
                    vertex.boneIndices[w] = static_cast<uint8_t>(weights[w].first);
                    totalWeight += weights[w].second;
                }
                
                // Normalize weights
                if (totalWeight > 0.0f) {
                    vertex.weights.x /= totalWeight;
                    vertex.weights.y /= totalWeight;
                    vertex.weights.z /= totalWeight;
                    vertex.weights.w /= totalWeight;
                }
            } else {
                // Default: bind to bone 0 with full weight
                vertex.weights.x = 1.0f;
                vertex.weights.y = 0.0f;
                vertex.weights.z = 0.0f;
                vertex.weights.w = 0.0f;
                vertex.boneIndices[0] = 0;
                vertex.boneIndices[1] = 0;
                vertex.boneIndices[2] = 0;
                vertex.boneIndices[3] = 0;
            }
            
            vertices.push_back(vertex);
            polyVertices.push_back(baseVertexIndex + vertexCounter);
            vertexCounter++;
        }
        
        // Now add indices based on polygon type
        if (polySize == 3) {
            // Triangle
            indices.push_back(polyVertices[0]);
            indices.push_back(polyVertices[1]);
            indices.push_back(polyVertices[2]);
        } else if (polySize == 4) {
            // Quad - split into two triangles
            indices.push_back(polyVertices[0]);
            indices.push_back(polyVertices[1]);
            indices.push_back(polyVertices[2]);
            
            indices.push_back(polyVertices[0]);
            indices.push_back(polyVertices[2]);
            indices.push_back(polyVertices[3]);
        }
        // Ignore polygons with more than 4 vertices for now
    }
    
    // Append vertices and indices to the mesh
    mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());
    mesh.indices.insert(mesh.indices.end(), indices.begin(), indices.end());
    
    
    // Output bounds for debugging
    if (!vertices.empty()) {
        float minX = vertices[0].pos.x, maxX = vertices[0].pos.x;
        float minY = vertices[0].pos.y, maxY = vertices[0].pos.y;
        float minZ = vertices[0].pos.z, maxZ = vertices[0].pos.z;
        
        for (const auto& v : vertices) {
            minX = (std::min)(minX, v.pos.x);
            maxX = (std::max)(maxX, v.pos.x);
            minY = (std::min)(minY, v.pos.y);
            maxY = (std::max)(maxY, v.pos.y);
            minZ = (std::min)(minZ, v.pos.z);
            maxZ = (std::max)(maxZ, v.pos.z);
        }
        
    }
    
    // Don't create buffers here - wait until all meshes are processed
}

void FbxLoader::ExtractMaterials(FbxNode* node, SkinMesh& mesh, IDirect3DDevice9* device) const {
    int materialCount = node->GetMaterialCount();
    
    // Don't clear materials - append to existing
    // mesh.materials.clear();
    mesh.materials.reserve(mesh.materials.size() + materialCount);
    
    for (int i = 0; i < materialCount; ++i) {
        FbxSurfaceMaterial* fbxMaterial = node->GetMaterial(i);
        if (!fbxMaterial) continue;
        
        Material material = {};
        
        // Extract material properties - simplified version
        // Use default material properties for now
        material.mat.Ambient = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
        material.mat.Diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
        material.mat.Specular = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
        material.mat.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
        material.mat.Power = 10.0f;
        
        // Extract texture - simplified version to avoid FBX SDK static member issues
        // TODO: Implement texture extraction when FBX SDK is properly configured
        material.tex = nullptr;
        
        mesh.materials.push_back(material);
    }
    
    // If no materials, add a default one
    if (mesh.materials.empty()) {
        Material defaultMat = {};
        defaultMat.mat.Ambient = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
        defaultMat.mat.Diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
        defaultMat.mat.Specular = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
        defaultMat.mat.Emissive = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
        defaultMat.mat.Power = 1.0f;
        defaultMat.tex = nullptr;
        mesh.materials.push_back(defaultMat);
    }
}

void FbxLoader::ExtractSkeleton(FbxNode* node, Skeleton& skel) const {
    if (!node) return;
    
    // Gather skeleton joints
    std::vector<FbxNode*> bones;
    std::function<void(FbxNode*)> gatherBones = [&](FbxNode* n) {
        if (n->GetNodeAttribute() && 
            n->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
            bones.push_back(n);
        }
        for (int i = 0; i < n->GetChildCount(); ++i) {
            gatherBones(n->GetChild(i));
        }
    };
    
    gatherBones(node);
    
    // Build joint hierarchy
    size_t jointCount = bones.size();
    skel.joints.resize(jointCount);
    
    for (size_t i = 0; i < jointCount; ++i) {
        FbxNode* boneNode = bones[i];
        skel.joints[i].name = boneNode->GetName();
        skel.joints[i].parentIndex = -1;
        
        // Find parent index
        for (size_t p = 0; p < jointCount; ++p) {
            if (bones[p] == boneNode->GetParent()) {
                skel.joints[i].parentIndex = static_cast<int>(p);
                break;
            }
        }
        
        // Get local transform
        FbxAMatrix localTransform = boneNode->EvaluateLocalTransform();
        FbxVector4 t = localTransform.GetT();
        FbxQuaternion q = localTransform.GetQ();
        FbxVector4 s = localTransform.GetS();
        
        // Convert to DirectX math
        DirectX::XMMATRIX mat = DirectX::XMMatrixScaling(
            static_cast<float>(s[0]),
            static_cast<float>(s[1]),
            static_cast<float>(s[2])
        ) * DirectX::XMMatrixRotationQuaternion(
            DirectX::XMVectorSet(
                static_cast<float>(q[0]),
                static_cast<float>(q[1]),
                static_cast<float>(q[2]),
                static_cast<float>(q[3])
            )
        ) * DirectX::XMMatrixTranslation(
            static_cast<float>(t[0]),
            static_cast<float>(t[1]),
            static_cast<float>(t[2])
        );
        
        DirectX::XMStoreFloat4x4(&skel.joints[i].bindPoseInverse, mat);
    }
    
    // TODO: Extract animations from FBX
    // For now, leave animations empty
}

void FbxLoader::ExtractSkinWeights(FbxMesh* fbxMesh, std::vector<std::vector<std::pair<int, float>>>& skinWeights) const {
    if (!fbxMesh) return;
    
    int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinCount == 0) {
        return;
    }
    
    // Process first skin deformer
    FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!skin) return;
    
    int clusterCount = skin->GetClusterCount();
    
    // Process each cluster (bone)
    for (int clusterIdx = 0; clusterIdx < clusterCount; ++clusterIdx) {
        FbxCluster* cluster = skin->GetCluster(clusterIdx);
        if (!cluster) continue;
        
        // Get the bone index (for now, use cluster index)
        int boneIndex = clusterIdx;
        
        // Get the indices and weights
        int indexCount = cluster->GetControlPointIndicesCount();
        int* indices = cluster->GetControlPointIndices();
        double* weights = cluster->GetControlPointWeights();
        
        // Add weights to the corresponding control points
        for (int i = 0; i < indexCount; ++i) {
            int vertexIndex = indices[i];
            float weight = static_cast<float>(weights[i]);
            
            if (vertexIndex >= 0 && vertexIndex < skinWeights.size() && weight > 0.0f) {
                skinWeights[vertexIndex].push_back({boneIndex, weight});
            }
        }
    }
    
    // Debug output
    int skinnedVertices = 0;
    for (const auto& weights : skinWeights) {
        if (!weights.empty()) skinnedVertices++;
    }
}

void FbxLoader::ExtractAnimations(FbxScene* scene, Skeleton& skel) const {
    if (!scene) return;
    
    // For now, skip animation extraction due to FBX SDK linking issues
    // TODO: Implement animation extraction when FBX SDK is properly configured
    
    // Create a simple test animation for demonstration
    if (!skel.joints.empty()) {
        SkeletonAnimation testAnim;
        testAnim.name = "test_animation";
        testAnim.duration = 2.0f; // 2 second loop
        
        // Initialize channels for each joint
        testAnim.channels.resize(skel.joints.size());
        
        // Add simple rotation animation for each joint
        for (size_t i = 0; i < skel.joints.size(); ++i) {
            // Add two keyframes for a simple rotation
            SkeletonAnimationKey key1, key2;
            key1.time = 0.0f;
            DirectX::XMStoreFloat4x4(&key1.transform, DirectX::XMMatrixIdentity());
            
            key2.time = 2.0f;
            // Small rotation around Y axis
            float angle = DirectX::XM_PI * 0.1f * (i % 3); // Different rotation per joint
            DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(angle);
            DirectX::XMStoreFloat4x4(&key2.transform, rotation);
            
            testAnim.channels[i].push_back(key1);
            testAnim.channels[i].push_back(key2);
        }
        
        skel.animations.push_back(testAnim);
    }
}