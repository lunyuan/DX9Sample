#pragma once
#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "ModelData.h"

// Model saving options
struct ModelSaveOptions {
    // General options
    bool embedTextures = false;          // Embed textures in file (if supported)
    bool compressData = true;            // Use compression (if supported)
    bool includeAnimations = true;       // Include animation data
    bool includeMaterials = true;        // Include material data
    bool generateTangents = false;       // Generate tangents/bitangents
    
    // Texture options
    std::string textureFormat = "";      // Convert textures to format (empty = keep original)
    int textureQuality = 95;             // JPEG quality or similar
    bool copyTextures = true;            // Copy texture files to output directory
    
    // Optimization options
    bool optimizeMeshes = false;         // Optimize vertex cache, remove duplicates
    bool stripUnusedBones = true;        // Remove bones with no weights
    float vertexWeldThreshold = 0.0001f; // Distance for welding vertices
    
    // Metadata
    std::string authorName;
    std::string copyright;
    std::string comments;
    std::string applicationName = "DX9Sample";
    
    // Format-specific options
    std::map<std::string, std::string> customOptions;
};

// Save result information
struct ModelSaveResult {
    bool success = false;
    std::string errorMessage;
    size_t bytesWritten = 0;
    std::vector<std::string> textureFiles; // External texture files created
    std::vector<std::string> warnings;
};

// Capabilities that a saver supports
struct ModelSaveCapabilities {
    bool supportsAnimation = false;
    bool supportsSkeletalAnimation = false;
    bool supportsMorphTargets = false;
    bool supportsPBRMaterials = false;
    bool supportsMultipleUVSets = false;
    bool supportsVertexColors = false;
    bool supportsEmbeddedTextures = false;
    bool supportsCompression = false;
    bool supportsSceneHierarchy = false;
    bool supportsMetadata = true;
    size_t maxBonesPerVertex = 4;
    std::vector<std::string> supportedTextureFormats;
};

// Model saver interface
struct IModelSaver {
    virtual ~IModelSaver() = default;
    
    // Save a single model
    [[nodiscard]] virtual ModelSaveResult SaveModel(
        const ModelData& model,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) = 0;
    
    // Save multiple models to a single file
    [[nodiscard]] virtual ModelSaveResult SaveAll(
        const std::map<std::string, ModelData>& models,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) = 0;
    
    // Check if the model can be saved in this format
    [[nodiscard]] virtual bool CanSave(const ModelData& model) const = 0;
    
    // Check if multiple models can be saved to one file
    [[nodiscard]] virtual bool SupportsMultipleModels() const = 0;
    
    // Get saver capabilities
    [[nodiscard]] virtual ModelSaveCapabilities GetCapabilities() const = 0;
    
    // Get supported file extensions
    [[nodiscard]] virtual std::vector<std::string> GetSupportedExtensions() const = 0;
    
    // Validate that save options are compatible
    [[nodiscard]] virtual bool ValidateOptions(const ModelSaveOptions& options) const = 0;
    
    // Estimate file size before saving
    [[nodiscard]] virtual size_t EstimateFileSize(
        const ModelData& model,
        const ModelSaveOptions& options) const = 0;
    
    // Get format-specific option descriptions
    [[nodiscard]] virtual std::map<std::string, std::string> GetCustomOptionDescriptions() const {
        return {};
    }
};

// Factory function type for creating savers
using ModelSaverFactory = std::unique_ptr<IModelSaver>(*)();