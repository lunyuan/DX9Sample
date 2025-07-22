#pragma once
#include <filesystem>
#include <memory>
#include <map>
#include <functional>
#include "IModelLoaderV2.h"
#include "IModelSaver.h"

// Model format conversion options
struct ConversionOptions {
    ModelLoadOptions loadOptions;
    ModelSaveOptions saveOptions;
    
    // Conversion-specific options
    bool preserveHierarchy = true;
    bool mergeIdenticalMaterials = true;
    bool generateMissingNormals = true;
    bool generateMissingTangents = false;
    bool convertToLeftHanded = false; // For Unity/Unreal compatibility
    bool convertUnits = false;
    float targetUnitScale = 1.0f; // Target units per meter
    
    // Progress callback
    std::function<void(float progress, const std::string& status)> progressCallback;
};

// Conversion result
struct ConversionResult {
    bool success = false;
    std::string errorMessage;
    std::vector<std::string> warnings;
    size_t modelsConverted = 0;
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    std::chrono::milliseconds conversionTime;
};

// Model format converter class
class ModelFormatConverter {
public:
    ModelFormatConverter();
    ~ModelFormatConverter();
    
    // Register loaders and savers
    void RegisterLoader(const std::string& extension, ModelLoaderFactory factory);
    void RegisterSaver(const std::string& extension, ModelSaverFactory factory);
    
    // Convert single file
    ConversionResult ConvertFile(
        const std::filesystem::path& inputFile,
        const std::filesystem::path& outputFile,
        const ConversionOptions& options = {});
    
    // Convert multiple files
    ConversionResult ConvertBatch(
        const std::vector<std::filesystem::path>& inputFiles,
        const std::filesystem::path& outputDirectory,
        const std::string& outputExtension,
        const ConversionOptions& options = {});
    
    // Check if conversion is supported
    bool CanConvert(
        const std::filesystem::path& inputFile,
        const std::filesystem::path& outputFile) const;
    
    // Get supported input formats
    std::vector<std::string> GetSupportedInputFormats() const;
    
    // Get supported output formats
    std::vector<std::string> GetSupportedOutputFormats() const;
    
    // Validate conversion
    std::vector<std::string> ValidateConversion(
        const std::filesystem::path& inputFile,
        const std::filesystem::path& outputFile,
        const ConversionOptions& options = {}) const;

private:
    // Loader/Saver factories
    std::map<std::string, ModelLoaderFactory> loaderFactories_;
    std::map<std::string, ModelSaverFactory> saverFactories_;
    
    // Cached loaders/savers
    mutable std::map<std::string, std::unique_ptr<IModelLoaderV2>> loaders_;
    mutable std::map<std::string, std::unique_ptr<IModelSaver>> savers_;
    
    // Helper methods
    IModelLoaderV2* GetLoader(const std::string& extension) const;
    IModelSaver* GetSaver(const std::string& extension) const;
    
    void ApplyConversionTransforms(
        ModelData& model,
        const ConversionOptions& options);
    
    void ConvertCoordinateSystem(
        ModelData& model,
        bool toLeftHanded);
    
    void ConvertUnits(
        ModelData& model,
        float fromScale,
        float toScale);
    
    void OptimizeModel(
        ModelData& model,
        const ConversionOptions& options);
    
    std::string NormalizeExtension(const std::string& ext) const;
};

// Global converter instance
ModelFormatConverter& GetModelFormatConverter();

// Convenience functions
inline ConversionResult ConvertModel(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputFile,
    const ConversionOptions& options = {}) {
    return GetModelFormatConverter().ConvertFile(inputFile, outputFile, options);
}

// Pre-configured conversion functions
ConversionResult ConvertToGltf(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputFile,
    bool binary = true);

ConversionResult ConvertToFbx(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputFile);

ConversionResult ConvertToX(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputFile);