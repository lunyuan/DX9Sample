#pragma once
#include "IModelSaver.h"
#include "tiny_gltf.h"
#include <memory>

// glTF 2.0 model saver implementation
class GltfSaver : public IModelSaver {
public:
    GltfSaver() = default;
    ~GltfSaver() override = default;
    
    // IModelSaver implementation
    [[nodiscard]] ModelSaveResult SaveModel(
        const ModelData& model,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) override;
    
    [[nodiscard]] ModelSaveResult SaveAll(
        const std::map<std::string, ModelData>& models,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) override;
    
    [[nodiscard]] bool CanSave(const ModelData& model) const override;
    
    [[nodiscard]] bool SupportsMultipleModels() const override { return true; }
    
    [[nodiscard]] ModelSaveCapabilities GetCapabilities() const override;
    
    [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override;
    
    [[nodiscard]] bool ValidateOptions(const ModelSaveOptions& options) const override;
    
    [[nodiscard]] size_t EstimateFileSize(
        const ModelData& model,
        const ModelSaveOptions& options) const override;
    
    [[nodiscard]] std::map<std::string, std::string> GetCustomOptionDescriptions() const override;

private:
    // Helper methods
    void ConvertMesh(
        const SkinMesh& mesh,
        tinygltf::Model& gltfModel,
        tinygltf::Mesh& gltfMesh,
        const ModelSaveOptions& options);
    
    void ConvertMaterial(
        const Material& material,
        tinygltf::Model& gltfModel,
        tinygltf::Material& gltfMaterial,
        const ModelSaveOptions& options);
    
    void ConvertSkeleton(
        const Skeleton& skeleton,
        tinygltf::Model& gltfModel,
        tinygltf::Skin& gltfSkin);
    
    void ConvertAnimation(
        const AnimationClip& animation,
        const Skeleton& skeleton,
        tinygltf::Model& gltfModel,
        tinygltf::Animation& gltfAnimation);
    
    void ConvertTexture(
        IDirect3DTexture9* texture,
        const std::string& name,
        tinygltf::Model& gltfModel,
        tinygltf::Texture& gltfTexture,
        const ModelSaveOptions& options);
    
    // Buffer management
    size_t AddBuffer(
        tinygltf::Model& model,
        const void* data,
        size_t size,
        const std::string& name = "");
    
    size_t AddBufferView(
        tinygltf::Model& model,
        size_t bufferIndex,
        size_t offset,
        size_t size,
        int target = 0); // ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER
    
    size_t AddAccessor(
        tinygltf::Model& model,
        size_t bufferViewIndex,
        size_t offset,
        int componentType,
        size_t count,
        const std::string& type,
        const std::vector<double>& min = {},
        const std::vector<double>& max = {});
    
    // Utility
    std::string GetMimeType(const std::string& extension) const;
    void CalculateMinMax(
        const void* data,
        size_t count,
        int componentType,
        int numComponents,
        std::vector<double>& min,
        std::vector<double>& max);
};

// Factory function
std::unique_ptr<IModelSaver> CreateGltfSaver();