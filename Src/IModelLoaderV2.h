#pragma once
#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "ModelData.h"

// Forward declaration
struct ModelInfo;

// Model loading options
struct ModelLoadOptions {
    bool loadTextures = true;
    bool loadAnimations = true;
    bool generateTangents = false;
    bool optimizeMeshes = false;
    bool flipUVs = false;
    bool flipWindingOrder = false;
    float scaleFactor = 1.0f;
    std::string preferredTextureFormat = ""; // Empty = use original
};

// Capabilities that a loader supports
struct ModelCapabilities {
    bool supportsAnimation = false;
    bool supportsSkeletalAnimation = false;
    bool supportsMorphTargets = false;
    bool supportsPBRMaterials = false;
    bool supportsMultipleUVSets = false;
    bool supportsVertexColors = false;
    bool supportsMultipleMeshes = true;
    bool supportsSceneHierarchy = false;
    size_t maxBonesPerVertex = 4;
    size_t maxTextureSize = 4096;
};

// Enhanced model loader interface
struct IModelLoaderV2 {
    virtual ~IModelLoaderV2() = default;
    
    // Load a specific model from file
    [[nodiscard]] virtual std::unique_ptr<ModelData> LoadModel(
        const std::filesystem::path& file,
        const std::string& modelName,
        IDirect3DDevice9* device,
        const ModelLoadOptions& options = {}) = 0;
    
    // Load all models from file
    [[nodiscard]] virtual std::map<std::string, std::unique_ptr<ModelData>> LoadAll(
        const std::filesystem::path& file,
        IDirect3DDevice9* device,
        const ModelLoadOptions& options = {}) = 0;
    
    // Get list of model names without loading
    [[nodiscard]] virtual std::vector<std::string> GetModelNames(
        const std::filesystem::path& file) const = 0;
    
    // Get detailed model information without full loading
    [[nodiscard]] virtual std::vector<ModelInfo> GetModelInfoList(
        const std::filesystem::path& file) const {
        // Default implementation - just returns names
        std::vector<ModelInfo> infos;
        auto names = GetModelNames(file);
        for (size_t i = 0; i < names.size(); ++i) {
            ModelInfo info;
            info.name = names[i];
            info.index = i;
            infos.push_back(info);
        }
        return infos;
    }
    
    // Check if this loader can handle the file
    [[nodiscard]] virtual bool CanLoad(
        const std::filesystem::path& file) const = 0;
    
    // Get loader capabilities
    [[nodiscard]] virtual ModelCapabilities GetCapabilities() const = 0;
    
    // Get supported file extensions
    [[nodiscard]] virtual std::vector<std::string> GetSupportedExtensions() const = 0;
    
    // Validate file before loading
    [[nodiscard]] virtual bool ValidateFile(
        const std::filesystem::path& file) const = 0;
    
    // Get estimated memory usage for loading
    [[nodiscard]] virtual size_t EstimateMemoryUsage(
        const std::filesystem::path& file) const = 0;
};

// Factory function type for creating loaders
using ModelLoaderFactory = std::unique_ptr<IModelLoaderV2>(*)();