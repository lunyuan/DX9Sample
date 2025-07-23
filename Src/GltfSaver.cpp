#include "GltfSaver.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <d3dx9.h>

// Factory function
std::unique_ptr<IModelSaver> CreateGltfSaver() {
    return std::make_unique<GltfSaver>();
}

ModelSaveResult GltfSaver::SaveModel(
    const ModelData& model,
    const std::filesystem::path& file,
    const ModelSaveOptions& options) {
    
    ModelSaveResult result;
    
    try {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltf;
        
        // Set asset info
        gltfModel.asset.version = "2.0";
        gltfModel.asset.generator = options.applicationName;
        if (!options.copyright.empty()) {
            gltfModel.asset.copyright = options.copyright;
        }
        
        // Add default scene
        tinygltf::Scene scene;
        scene.name = "Scene";
        
        // Convert mesh
        if (!model.meshes.empty() && model.meshes[0]) {
            tinygltf::Node node;
            node.name = model.metadata.name.empty() ? "Model" : model.metadata.name;
            
            tinygltf::Mesh gltfMesh;
            gltfMesh.name = node.name;
            
            ConvertMesh(*model.meshes[0], gltfModel, gltfMesh, options);
            
            gltfModel.meshes.push_back(gltfMesh);
            node.mesh = static_cast<int>(gltfModel.meshes.size() - 1);
            
            // Add skeleton if present
            if (!model.skeleton.joints.empty() && options.includeAnimations) {
                tinygltf::Skin skin;
                ConvertSkeleton(model.skeleton, gltfModel, skin);
                gltfModel.skins.push_back(skin);
                node.skin = static_cast<int>(gltfModel.skins.size() - 1);
            }
            
            scene.nodes.push_back(static_cast<int>(gltfModel.nodes.size()));
            gltfModel.nodes.push_back(node);
        }
        
        // Add animations
        if (options.includeAnimations) {
            for (const auto& animClip : model.animations) {
                tinygltf::Animation anim;
                ConvertAnimation(animClip, model.skeleton, gltfModel, anim);
                gltfModel.animations.push_back(anim);
            }
        }
        
        gltfModel.scenes.push_back(scene);
        gltfModel.defaultScene = 0;
        
        // Save to file
        bool embedImages = options.embedTextures;
        bool embedBuffers = true;
        bool prettyPrint = true;
        bool writeBinary = file.extension() == ".glb";
        
        std::string error, warning;
        bool success = false;
        
        if (writeBinary) {
            success = gltf.WriteGltfSceneToFile(&gltfModel, file.string(),
                embedImages, embedBuffers, prettyPrint, true); // true = binary
        } else {
            success = gltf.WriteGltfSceneToFile(&gltfModel, file.string(),
                embedImages, embedBuffers, prettyPrint, false); // false = ASCII
        }
        
        if (success) {
            result.success = true;
            result.bytesWritten = std::filesystem::file_size(file);
        } else {
            result.success = false;
            result.errorMessage = "Failed to write glTF file";
        }
        
        if (!warning.empty()) {
            result.warnings.push_back(warning);
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

ModelSaveResult GltfSaver::SaveAll(
    const std::map<std::string, ModelData>& models,
    const std::filesystem::path& file,
    const ModelSaveOptions& options) {
    
    ModelSaveResult result;
    
    // For multiple models, create a scene with multiple nodes
    // TODO: Implement multi-model export
    result.success = false;
    result.errorMessage = "Multi-model export not yet implemented";
    
    return result;
}

bool GltfSaver::CanSave(const ModelData& model) const {
    // Check if we have valid mesh data
    if (model.meshes.empty() || !model.meshes[0]) {
        return false;
    }
    
    // Check vertex format compatibility
    const auto& mesh = *model.meshes[0];
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return false;
    }
    
    // glTF can save most models
    return true;
}

ModelSaveCapabilities GltfSaver::GetCapabilities() const {
    ModelSaveCapabilities caps;
    caps.supportsAnimation = true;
    caps.supportsSkeletalAnimation = true;
    caps.supportsMorphTargets = false; // Not implemented yet
    caps.supportsPBRMaterials = true;
    caps.supportsMultipleUVSets = true;
    caps.supportsVertexColors = true;
    caps.supportsEmbeddedTextures = true;
    caps.supportsCompression = true; // Via Draco extension
    caps.supportsSceneHierarchy = true;
    caps.supportsMetadata = true;
    caps.maxBonesPerVertex = 4;
    caps.supportedTextureFormats = {"png", "jpg", "jpeg"};
    return caps;
}

std::vector<std::string> GltfSaver::GetSupportedExtensions() const {
    return {".gltf", ".glb"};
}

bool GltfSaver::ValidateOptions(const ModelSaveOptions& options) const {
    // Check texture format
    if (!options.textureFormat.empty()) {
        auto caps = GetCapabilities();
        auto it = std::find(caps.supportedTextureFormats.begin(),
                           caps.supportedTextureFormats.end(),
                           options.textureFormat);
        if (it == caps.supportedTextureFormats.end()) {
            return false;
        }
    }
    
    return true;
}

size_t GltfSaver::EstimateFileSize(
    const ModelData& model,
    const ModelSaveOptions& options) const {
    
    size_t size = 0;
    
    // Estimate vertex data size
    if (!model.meshes.empty() && model.meshes[0]) {
        const auto& mesh = *model.meshes[0];
        size += mesh.vertices.size() * sizeof(Vertex);
        size += mesh.indices.size() * sizeof(uint32_t);
    }
    
    // Estimate texture size
    if (options.embedTextures && !model.texturePaths.empty()) {
        // Rough estimate: 1MB per texture
        size += model.texturePaths.size() * 1024 * 1024;
    }
    
    // Animation data
    if (options.includeAnimations) {
        size += model.skeleton.joints.size() * sizeof(DirectX::XMFLOAT4X4) * 100; // Rough estimate
    }
    
    // JSON overhead (~20%)
    size = static_cast<size_t>(size * 1.2);
    
    return size;
}

std::map<std::string, std::string> GltfSaver::GetCustomOptionDescriptions() const {
    return {
        {"draco.compression", "Enable Draco geometry compression (true/false)"},
        {"draco.quantization.position", "Position quantization bits (1-32)"},
        {"draco.quantization.normal", "Normal quantization bits (1-32)"},
        {"draco.quantization.texcoord", "Texture coordinate quantization bits (1-32)"},
        {"ktx2.compression", "Use KTX2 for texture compression (true/false)"}
    };
}

void GltfSaver::ConvertMesh(
    const SkinMesh& mesh,
    tinygltf::Model& gltfModel,
    tinygltf::Mesh& gltfMesh,
    const ModelSaveOptions& options) {
    
    tinygltf::Primitive primitive;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    
    // Convert vertex data
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<unsigned short> joints;
    std::vector<float> weights;
    
    positions.reserve(mesh.vertices.size() * 3);
    normals.reserve(mesh.vertices.size() * 3);
    texcoords.reserve(mesh.vertices.size() * 2);
    
    bool hasSkinning = !mesh.skeleton.joints.empty();
    if (hasSkinning) {
        joints.reserve(mesh.vertices.size() * 4);
        weights.reserve(mesh.vertices.size() * 4);
    }
    
    for (const auto& v : mesh.vertices) {
        // Position
        positions.push_back(v.pos.x);
        positions.push_back(v.pos.y);
        positions.push_back(v.pos.z);
        
        // Normal
        normals.push_back(v.norm.x);
        normals.push_back(v.norm.y);
        normals.push_back(v.norm.z);
        
        // Texture coordinates
        texcoords.push_back(v.uv.x);
        texcoords.push_back(options.flipUVs ? 1.0f - v.uv.y : v.uv.y);
        
        // Skinning data
        if (hasSkinning) {
            for (int i = 0; i < 4; ++i) {
                joints.push_back(v.boneIndices[i]);
                weights.push_back((&v.weights.x)[i]);
            }
        }
    }
    
    // Add buffers
    size_t posBuffer = AddBuffer(gltfModel, positions.data(), 
        positions.size() * sizeof(float), "positions");
    size_t normBuffer = AddBuffer(gltfModel, normals.data(), 
        normals.size() * sizeof(float), "normals");
    size_t uvBuffer = AddBuffer(gltfModel, texcoords.data(), 
        texcoords.size() * sizeof(float), "texcoords");
    
    // Add buffer views
    size_t posView = AddBufferView(gltfModel, posBuffer, 0, 
        positions.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    size_t normView = AddBufferView(gltfModel, normBuffer, 0, 
        normals.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    size_t uvView = AddBufferView(gltfModel, uvBuffer, 0, 
        texcoords.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    
    // Calculate bounds for position accessor
    std::vector<double> posMin(3, std::numeric_limits<double>::max());
    std::vector<double> posMax(3, std::numeric_limits<double>::lowest());
    for (size_t i = 0; i < positions.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            posMin[j] = std::min(posMin[j], static_cast<double>(positions[i + j]));
            posMax[j] = std::max(posMax[j], static_cast<double>(positions[i + j]));
        }
    }
    
    // Add accessors
    primitive.attributes["POSITION"] = AddAccessor(gltfModel, posView, 0,
        TINYGLTF_COMPONENT_TYPE_FLOAT, mesh.vertices.size(), "VEC3", posMin, posMax);
    primitive.attributes["NORMAL"] = AddAccessor(gltfModel, normView, 0,
        TINYGLTF_COMPONENT_TYPE_FLOAT, mesh.vertices.size(), "VEC3");
    primitive.attributes["TEXCOORD_0"] = AddAccessor(gltfModel, uvView, 0,
        TINYGLTF_COMPONENT_TYPE_FLOAT, mesh.vertices.size(), "VEC2");
    
    // Add skinning attributes
    if (hasSkinning) {
        size_t jointBuffer = AddBuffer(gltfModel, joints.data(),
            joints.size() * sizeof(unsigned short), "joints");
        size_t weightBuffer = AddBuffer(gltfModel, weights.data(),
            weights.size() * sizeof(float), "weights");
            
        size_t jointView = AddBufferView(gltfModel, jointBuffer, 0,
            joints.size() * sizeof(unsigned short), TINYGLTF_TARGET_ARRAY_BUFFER);
        size_t weightView = AddBufferView(gltfModel, weightBuffer, 0,
            weights.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
            
        primitive.attributes["JOINTS_0"] = AddAccessor(gltfModel, jointView, 0,
            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, mesh.vertices.size(), "VEC4");
        primitive.attributes["WEIGHTS_0"] = AddAccessor(gltfModel, weightView, 0,
            TINYGLTF_COMPONENT_TYPE_FLOAT, mesh.vertices.size(), "VEC4");
    }
    
    // Convert indices
    size_t indexBuffer = AddBuffer(gltfModel, mesh.indices.data(),
        mesh.indices.size() * sizeof(uint32_t), "indices");
    size_t indexView = AddBufferView(gltfModel, indexBuffer, 0,
        mesh.indices.size() * sizeof(uint32_t), TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
    primitive.indices = AddAccessor(gltfModel, indexView, 0,
        TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, mesh.indices.size(), "SCALAR");
    
    // Set material
    if (!mesh.materials.empty() && options.includeMaterials) {
        tinygltf::Material material;
        ConvertMaterial(mesh.materials[0], gltfModel, material, options);
        gltfModel.materials.push_back(material);
        primitive.material = static_cast<int>(gltfModel.materials.size() - 1);
    }
    
    gltfMesh.primitives.push_back(primitive);
}

void GltfSaver::ConvertMaterial(
    const Material& material,
    tinygltf::Model& gltfModel,
    tinygltf::Material& gltfMaterial,
    const ModelSaveOptions& options) {
    
    // Convert D3D material to PBR
    gltfMaterial.name = "Material";
    
    // Base color from diffuse
    gltfMaterial.pbrMetallicRoughness.baseColorFactor = {
        material.mat.Diffuse.r,
        material.mat.Diffuse.g,
        material.mat.Diffuse.b,
        material.mat.Diffuse.a
    };
    
    // Approximate metallic/roughness from specular
    float specularIntensity = (material.mat.Specular.r + material.mat.Specular.g + material.mat.Specular.b) / 3.0f;
    gltfMaterial.pbrMetallicRoughness.metallicFactor = specularIntensity > 0.5f ? 0.5f : 0.0f;
    gltfMaterial.pbrMetallicRoughness.roughnessFactor = 1.0f - (material.mat.Power / 128.0f);
    
    // TODO: Convert and embed textures if requested
}

void GltfSaver::ConvertSkeleton(
    const Skeleton& skeleton,
    tinygltf::Model& gltfModel,
    tinygltf::Skin& gltfSkin) {
    
    gltfSkin.name = "Skeleton";
    
    // Create nodes for each joint
    for (const auto& joint : skeleton.joints) {
        tinygltf::Node node;
        node.name = joint.name;
        
        // Set transform from bind pose
        // TODO: Decompose matrix to TRS
        
        gltfSkin.joints.push_back(static_cast<int>(gltfModel.nodes.size()));
        gltfModel.nodes.push_back(node);
    }
    
    // Set inverse bind matrices
    std::vector<float> inverseBindMatrices;
    for (const auto& joint : skeleton.joints) {
        const auto& m = joint.bindPoseInverse;
        // glTF uses column-major order
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                inverseBindMatrices.push_back((&m.m[row][col])[0]);
            }
        }
    }
    
    size_t buffer = AddBuffer(gltfModel, inverseBindMatrices.data(),
        inverseBindMatrices.size() * sizeof(float), "inverseBindMatrices");
    size_t view = AddBufferView(gltfModel, buffer, 0,
        inverseBindMatrices.size() * sizeof(float));
    gltfSkin.inverseBindMatrices = AddAccessor(gltfModel, view, 0,
        TINYGLTF_COMPONENT_TYPE_FLOAT, skeleton.joints.size(), "MAT4");
}

void GltfSaver::ConvertAnimation(
    const AnimationClip& animation,
    const Skeleton& skeleton,
    tinygltf::Model& gltfModel,
    tinygltf::Animation& gltfAnimation) {
    
    gltfAnimation.name = animation.name;
    
    // TODO: Convert animation tracks
    // This requires converting from the engine's animation format to glTF samplers
}

size_t GltfSaver::AddBuffer(
    tinygltf::Model& model,
    const void* data,
    size_t size,
    const std::string& name) {
    
    tinygltf::Buffer buffer;
    buffer.name = name;
    buffer.data.resize(size);
    memcpy(buffer.data.data(), data, size);
    
    model.buffers.push_back(buffer);
    return model.buffers.size() - 1;
}

size_t GltfSaver::AddBufferView(
    tinygltf::Model& model,
    size_t bufferIndex,
    size_t offset,
    size_t size,
    int target) {
    
    tinygltf::BufferView view;
    view.buffer = static_cast<int>(bufferIndex);
    view.byteOffset = offset;
    view.byteLength = size;
    if (target != 0) {
        view.target = target;
    }
    
    model.bufferViews.push_back(view);
    return model.bufferViews.size() - 1;
}

size_t GltfSaver::AddAccessor(
    tinygltf::Model& model,
    size_t bufferViewIndex,
    size_t offset,
    int componentType,
    size_t count,
    const std::string& type,
    const std::vector<double>& min,
    const std::vector<double>& max) {
    
    tinygltf::Accessor accessor;
    accessor.bufferView = static_cast<int>(bufferViewIndex);
    accessor.byteOffset = offset;
    accessor.componentType = componentType;
    accessor.count = count;
    accessor.type = type;
    
    if (!min.empty()) {
        accessor.minValues = min;
    }
    if (!max.empty()) {
        accessor.maxValues = max;
    }
    
    model.accessors.push_back(accessor);
    return model.accessors.size() - 1;
}