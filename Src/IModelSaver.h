#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <memory>
#include <chrono>
#include "ModelData.h"

// Model save options
struct ModelSaveOptions {
    bool embedTextures = false;
    bool compressData = false;
    bool prettyPrint = true;
    bool includeMetadata = true;
    std::string author;
    std::string copyright;
};

// Save result
struct ModelSaveResult {
    bool success = false;
    std::string error;
    size_t bytesWritten = 0;
    std::chrono::milliseconds saveTime;
};

// Model save capabilities
struct ModelSaveCapabilities {
    bool supportsBinary = true;
    bool supportsText = false;
    bool supportsAnimation = true;
    bool supportsMaterials = true;
    bool supportsTextures = true;
    bool supportsHierarchy = true;
    size_t maxTextureSize = 0;
    std::vector<std::string> supportedTextureFormats;
};

// Model saver interface
class IModelSaver {
public:
    virtual ~IModelSaver() = default;
    
    // Save single model
    virtual ModelSaveResult SaveModel(
        const ModelData& model,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) = 0;
    
    // Save multiple models
    virtual ModelSaveResult SaveAll(
        const std::map<std::string, ModelData>& models,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) = 0;
    
    // Get supported extensions
    virtual std::vector<std::string> GetSupportedExtensions() const = 0;
    
    // Check if can save model
    virtual bool CanSave(const ModelData& model) const = 0;
    
    // Check if supports multiple models
    virtual bool SupportsMultipleModels() const = 0;
    
    // Get capabilities
    virtual ModelSaveCapabilities GetCapabilities() const = 0;
    
    // Validate options
    virtual bool ValidateOptions(const ModelSaveOptions& options) const = 0;
    
    // Estimate file size
    virtual size_t EstimateFileSize(
        const ModelData& model,
        const ModelSaveOptions& options) const = 0;
};

// Factory function
using ModelSaverFactory = std::function<std::unique_ptr<IModelSaver>()>;

// FBX Saver factory
std::unique_ptr<IModelSaver> CreateFbxSaver();