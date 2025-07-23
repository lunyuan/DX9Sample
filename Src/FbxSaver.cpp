#include "FbxSaver.h"
#include <iostream>
#include <filesystem>
#include <windows.h>

// Factory function
std::unique_ptr<IModelSaver> CreateFbxSaver() {
    return std::make_unique<FbxSaver>();
}

FbxSaver::FbxSaver() {
    // Initialize FBX SDK
    fbxManager_ = FbxManager::Create();
    if (!fbxManager_) {
        throw std::runtime_error("Failed to create FBX Manager");
    }
    
    // Create IO settings
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager_, IOSROOT);
    fbxManager_->SetIOSettings(ios);
}

FbxSaver::~FbxSaver() {
    if (fbxManager_) {
        fbxManager_->Destroy();
    }
}

ModelSaveResult FbxSaver::SaveModel(
    const ModelData& model,
    const std::filesystem::path& file,
    const ModelSaveOptions& options) {
    
    ModelSaveResult result;
    
    try {
        // Create scene
        FbxScene* scene = FbxScene::Create(fbxManager_, "ExportScene");
        
        // Set scene info
        FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(fbxManager_, "SceneInfo");
        sceneInfo->mTitle = "DX9Sample Export";
        sceneInfo->mSubject = "3D Model Export";
        sceneInfo->mAuthor = options.author.c_str();
        sceneInfo->mRevision = "1.0";
        sceneInfo->mKeywords = "DX9Sample FBX Export";
        sceneInfo->mComment = options.copyright.c_str();
        sceneInfo->Original_ApplicationName.Set("DX9Sample");
        scene->SetSceneInfo(sceneInfo);
        
        // Create mesh node
        std::string modelName = file.stem().string();
        FbxNode* meshNode = CreateMeshNode(scene, modelName, model);
        scene->GetRootNode()->AddChild(meshNode);
        
        // Export scene
        result.success = ExportScene(scene, file, options);
        
        if (result.success) {
            result.bytesWritten = std::filesystem::file_size(file);
        } else {
            result.error = "Failed to export FBX file";
        }
        
        // Cleanup
        scene->Destroy();
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    
    return result;
}

ModelSaveResult FbxSaver::SaveAll(
    const std::map<std::string, ModelData>& models,
    const std::filesystem::path& file,
    const ModelSaveOptions& options) {
    
    ModelSaveResult result;
    
    try {
        // Create scene
        FbxScene* scene = FbxScene::Create(fbxManager_, "ExportScene");
        
        // Set scene info
        FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(fbxManager_, "SceneInfo");
        sceneInfo->mTitle = "DX9Sample Multi-Model Export";
        sceneInfo->mAuthor = options.author.c_str();
        sceneInfo->Original_ApplicationName.Set("DX9Sample");
        scene->SetSceneInfo(sceneInfo);
        
        // Create a node for each model
        for (const auto& [name, modelData] : models) {
            FbxNode* meshNode = CreateMeshNode(scene, name, modelData);
            scene->GetRootNode()->AddChild(meshNode);
        }
        
        // Export scene
        result.success = ExportScene(scene, file, options);
        
        if (result.success) {
            result.bytesWritten = std::filesystem::file_size(file);
            std::cout << "Exported " << models.size() << " models to " << file << std::endl;
        } else {
            result.error = "Failed to export FBX file";
        }
        
        // Cleanup
        scene->Destroy();
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    
    return result;
}

FbxNode* FbxSaver::CreateMeshNode(FbxScene* scene, const std::string& name, const ModelData& model) {
    // Create node
    FbxNode* node = FbxNode::Create(scene, name.c_str());
    
    // Create mesh
    FbxMesh* mesh = CreateFbxMesh(scene, model.mesh);
    
    // Apply materials
    ApplyMaterials(node, mesh, model.mesh, "");
    
    // Set mesh to node
    node->SetNodeAttribute(mesh);
    
    return node;
}

FbxMesh* FbxSaver::CreateFbxMesh(FbxScene* scene, const SkinMesh& skinMesh) {
    FbxMesh* mesh = FbxMesh::Create(scene, "mesh");
    
    // Set control points (vertices)
    mesh->InitControlPoints(static_cast<int>(skinMesh.vertices.size()));
    FbxVector4* controlPoints = mesh->GetControlPoints();
    
    for (size_t i = 0; i < skinMesh.vertices.size(); ++i) {
        const auto& v = skinMesh.vertices[i];
        controlPoints[i] = FbxVector4(v.pos.x, v.pos.y, v.pos.z);
    }
    
    // Create layer for normals
    FbxLayer* layer0 = mesh->GetLayer(0);
    if (!layer0) {
        mesh->CreateLayer();
        layer0 = mesh->GetLayer(0);
    }
    
    // Add normals
    FbxLayerElementNormal* layerNormal = FbxLayerElementNormal::Create(mesh, "");
    layerNormal->SetMappingMode(FbxLayerElement::eByControlPoint);
    layerNormal->SetReferenceMode(FbxLayerElement::eDirect);
    
    for (const auto& v : skinMesh.vertices) {
        layerNormal->GetDirectArray().Add(FbxVector4(v.norm.x, v.norm.y, v.norm.z));
    }
    layer0->SetNormals(layerNormal);
    
    // Add UVs
    FbxLayerElementUV* layerUV = FbxLayerElementUV::Create(mesh, "DiffuseUV");
    layerUV->SetMappingMode(FbxLayerElement::eByControlPoint);
    layerUV->SetReferenceMode(FbxLayerElement::eDirect);
    
    for (const auto& v : skinMesh.vertices) {
        layerUV->GetDirectArray().Add(FbxVector2(v.uv.x, 1.0 - v.uv.y)); // Flip V for FBX
    }
    layer0->SetUVs(layerUV, FbxLayerElement::eTextureDiffuse);
    
    // Add vertex colors
    FbxLayerElementVertexColor* layerColor = FbxLayerElementVertexColor::Create(mesh, "");
    layerColor->SetMappingMode(FbxLayerElement::eByControlPoint);
    layerColor->SetReferenceMode(FbxLayerElement::eDirect);
    
    for (const auto& v : skinMesh.vertices) {
        float r = ((v.col >> 16) & 0xFF) / 255.0f;
        float g = ((v.col >> 8) & 0xFF) / 255.0f;
        float b = (v.col & 0xFF) / 255.0f;
        float a = ((v.col >> 24) & 0xFF) / 255.0f;
        layerColor->GetDirectArray().Add(FbxColor(r, g, b, a));
    }
    layer0->SetVertexColors(layerColor);
    
    // Add polygons (triangles)
    for (size_t i = 0; i < skinMesh.indices.size(); i += 3) {
        mesh->BeginPolygon();
        mesh->AddPolygon(skinMesh.indices[i]);
        mesh->AddPolygon(skinMesh.indices[i + 1]);
        mesh->AddPolygon(skinMesh.indices[i + 2]);
        mesh->EndPolygon();
    }
    
    return mesh;
}

void FbxSaver::ApplyMaterials(FbxNode* node, FbxMesh* fbxMesh, const SkinMesh& mesh, const std::filesystem::path& basePath) {
    FbxScene* scene = node->GetScene();
    
    // Create material for each material in the mesh
    for (size_t i = 0; i < mesh.materials.size(); ++i) {
        const auto& mat = mesh.materials[i];
        
        std::string matName = "Material_" + std::to_string(i);
        FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, matName.c_str());
        
        // Set material properties
        material->Diffuse.Set(FbxDouble3(mat.mat.Diffuse.r, mat.mat.Diffuse.g, mat.mat.Diffuse.b));
        material->Ambient.Set(FbxDouble3(mat.mat.Ambient.r, mat.mat.Ambient.g, mat.mat.Ambient.b));
        material->Specular.Set(FbxDouble3(mat.mat.Specular.r, mat.mat.Specular.g, mat.mat.Specular.b));
        material->Emissive.Set(FbxDouble3(mat.mat.Emissive.r, mat.mat.Emissive.g, mat.mat.Emissive.b));
        material->Shininess.Set(mat.mat.Power);
        material->TransparencyFactor.Set(1.0 - mat.mat.Diffuse.a);
        
        // Export texture if we have the filename
        if (!mat.textureFileName.empty()) {
            FbxFileTexture* texture = FbxFileTexture::Create(scene, ("Texture_" + std::to_string(i)).c_str());
            texture->SetFileName(mat.textureFileName.c_str());
            texture->SetTextureUse(FbxTexture::eStandard);
            texture->SetMappingType(FbxTexture::eUV);
            texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
            texture->SetSwapUV(false);
            texture->SetTranslation(0.0, 0.0);
            texture->SetScale(1.0, 1.0);
            texture->SetRotation(0.0, 0.0);
            material->Diffuse.ConnectSrcObject(texture);
            
            char debugMsg[256];
            sprintf_s(debugMsg, "FbxSaver: Exported texture reference: %s\n", mat.textureFileName.c_str());
            OutputDebugStringA(debugMsg);
        }
        
        node->AddMaterial(material);
    }
    
    // If no materials, create a default one
    if (mesh.materials.empty()) {
        FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, "DefaultMaterial");
        material->Diffuse.Set(FbxDouble3(0.8, 0.8, 0.8));
        material->Ambient.Set(FbxDouble3(0.2, 0.2, 0.2));
        material->Specular.Set(FbxDouble3(0.0, 0.0, 0.0));
        node->AddMaterial(material);
    }
}

bool FbxSaver::ExportScene(FbxScene* scene, const std::filesystem::path& file, const ModelSaveOptions& options) {
    // Create exporter
    FbxExporter* exporter = FbxExporter::Create(fbxManager_, "");
    
    // Initialize exporter
    bool exportStatus = exporter->Initialize(file.string().c_str(), -1, fbxManager_->GetIOSettings());
    if (!exportStatus) {
        std::cerr << "Failed to initialize FBX exporter: " << exporter->GetStatus().GetErrorString() << std::endl;
        exporter->Destroy();
        return false;
    }
    
    // Set export options
    FbxIOSettings* ios = fbxManager_->GetIOSettings();
    ios->SetBoolProp(EXP_FBX_MATERIAL, true);
    ios->SetBoolProp(EXP_FBX_TEXTURE, true); // Default: include materials
    ios->SetBoolProp(EXP_FBX_EMBEDDED, options.embedTextures);
    ios->SetBoolProp(EXP_FBX_ANIMATION, true); // Default: include animations
    ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
    
    // Export scene
    bool status = exporter->Export(scene);
    
    if (!status) {
        std::cerr << "Failed to export FBX: " << exporter->GetStatus().GetErrorString() << std::endl;
    }
    
    exporter->Destroy();
    return status;
}

bool FbxSaver::CanSave(const ModelData& model) const {
    // Check if model has valid mesh data
    return !model.mesh.vertices.empty() && !model.mesh.indices.empty();
}

ModelSaveCapabilities FbxSaver::GetCapabilities() const {
    ModelSaveCapabilities caps;
    caps.supportsAnimation = true;
    // Additional capabilities (not in interface)
    // caps.supportsSkeletalAnimation = true;
    // caps.supportsMorphTargets = false;
    // caps.supportsPBRMaterials = false;
    // caps.supportsMultipleUVSets = true;
    // caps.supportsVertexColors = true;
    // caps.supportsEmbeddedTextures = true;
    // caps.supportsCompression = true;
    // caps.supportsSceneHierarchy = true;
    // caps.supportsMetadata = true;
    // caps.maxBonesPerVertex = 4;
    caps.supportedTextureFormats = { "jpg", "png", "tga", "bmp" };
    return caps;
}

bool FbxSaver::ValidateOptions(const ModelSaveOptions& options) const {
    // Basic validation
    return true;
}

size_t FbxSaver::EstimateFileSize(const ModelData& model, const ModelSaveOptions& options) const {
    // Rough estimate
    size_t vertexSize = model.mesh.vertices.size() * sizeof(Vertex);
    size_t indexSize = model.mesh.indices.size() * sizeof(uint32_t);
    size_t materialSize = model.mesh.materials.size() * 1024; // Rough estimate
    
    return (vertexSize + indexSize + materialSize) * 2; // FBX overhead
}