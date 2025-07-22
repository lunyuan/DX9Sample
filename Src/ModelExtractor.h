#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <regex>
#include "ModelDataV2.h"
#include "IModelLoaderV2.h"
#include "IModelSaver.h"

// Model selection criteria
struct ModelSelectionCriteria {
    // Selection by name
    std::vector<std::string> modelNames;        // Exact names
    std::vector<std::string> namePatterns;      // Regex patterns
    bool caseSensitive = false;
    
    // Selection by index
    std::vector<size_t> modelIndices;           // Direct indices
    
    // Selection by properties
    std::vector<std::string> requiredTags;      // Must have all these tags
    std::vector<std::string> excludeTags;       // Must not have these tags
    
    // Selection by mesh properties
    size_t minVertexCount = 0;
    size_t maxVertexCount = std::numeric_limits<size_t>::max();
    size_t minTriangleCount = 0;
    size_t maxTriangleCount = std::numeric_limits<size_t>::max();
    bool mustHaveAnimation = false;
    bool mustHaveSkeleton = false;
    
    // Selection by hierarchy
    std::string parentNodeName;                 // Select children of this node
    int maxDepth = -1;                          // Max depth from parent (-1 = unlimited)
    
    // Combination logic
    enum class CombineMode {
        AND,    // All criteria must match
        OR      // Any criteria can match
    } combineMode = CombineMode::OR;
};

// Model extraction options
struct ModelExtractionOptions {
    // What to include
    bool includeAnimations = true;
    bool includeMaterials = true;
    bool includeTextures = true;
    bool preserveHierarchy = true;
    
    // How to handle shared resources
    bool duplicateSharedMaterials = false;     // Each model gets its own copy
    bool duplicateSharedTextures = false;
    bool extractSharedSkeleton = true;         // Extract skeleton even if shared
    
    // Transformation options
    bool applyTransforms = false;              // Bake node transforms into vertices
    bool centerModels = false;                 // Center each model at origin
    bool normalizeScale = false;               // Scale to unit size
    
    // Naming options
    std::string namingPattern = "{original}";  // {original}, {index}, {parent}_{original}
    std::string namePrefix = "";
    std::string nameSuffix = "";
    
    // Memory optimization
    bool removeUnusedBones = true;
    bool removeUnusedMaterials = true;
    bool optimizeMeshes = false;
};

// Model information for preview/selection
struct ModelInfo {
    std::string name;
    size_t index;
    
    // Mesh statistics
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    size_t materialCount = 0;
    
    // Hierarchy info
    std::string parentNodeName;
    int hierarchyDepth = 0;
    std::vector<std::string> childNames;
    
    // Animation info
    bool hasAnimation = false;
    bool hasSkeleton = false;
    size_t boneCount = 0;
    std::vector<std::string> animationClips;
    
    // Bounds
    BoundingBox bounds;
    
    // Tags and metadata
    std::vector<std::string> tags;
    std::map<std::string, std::string> metadata;
};

// Model extractor class
class ModelExtractor {
public:
    ModelExtractor() = default;
    ~ModelExtractor() = default;
    
    // Load and get model information
    std::vector<ModelInfo> GetModelInfo(
        const std::filesystem::path& file,
        IModelLoaderV2* loader = nullptr);
    
    // Extract single model by name
    std::unique_ptr<ModelDataV2> ExtractModel(
        const std::filesystem::path& file,
        const std::string& modelName,
        const ModelExtractionOptions& options = {},
        IModelLoaderV2* loader = nullptr);
    
    // Extract single model by index
    std::unique_ptr<ModelDataV2> ExtractModelByIndex(
        const std::filesystem::path& file,
        size_t index,
        const ModelExtractionOptions& options = {},
        IModelLoaderV2* loader = nullptr);
    
    // Extract multiple models by criteria
    std::map<std::string, std::unique_ptr<ModelDataV2>> ExtractModels(
        const std::filesystem::path& file,
        const ModelSelectionCriteria& criteria,
        const ModelExtractionOptions& options = {},
        IModelLoaderV2* loader = nullptr);
    
    // Extract all models
    std::map<std::string, std::unique_ptr<ModelDataV2>> ExtractAllModels(
        const std::filesystem::path& file,
        const ModelExtractionOptions& options = {},
        IModelLoaderV2* loader = nullptr);
    
    // Split models into separate files
    bool SplitModelsToFiles(
        const std::filesystem::path& inputFile,
        const std::filesystem::path& outputDirectory,
        const ModelSelectionCriteria& criteria,
        const ModelExtractionOptions& options = {},
        IModelLoaderV2* loader = nullptr,
        IModelSaver* saver = nullptr);
    
    // Merge multiple models into one
    std::unique_ptr<ModelDataV2> MergeModels(
        const std::vector<std::unique_ptr<ModelDataV2>>& models,
        const std::string& mergedName = "MergedModel");
    
    // Preview extraction without loading full data
    std::vector<std::string> PreviewExtraction(
        const std::filesystem::path& file,
        const ModelSelectionCriteria& criteria,
        IModelLoaderV2* loader = nullptr);
    
    // Utility methods
    static bool MatchesCriteria(
        const ModelInfo& info,
        const ModelSelectionCriteria& criteria);
    
    static std::string GenerateModelName(
        const std::string& originalName,
        size_t index,
        const ModelExtractionOptions& options);

private:
    // Helper methods
    std::unique_ptr<ModelDataV2> ExtractFromLoadedModels(
        const std::map<std::string, std::unique_ptr<ModelDataV2>>& models,
        const std::string& modelName,
        const ModelExtractionOptions& options);
    
    void ExtractModelData(
        const ModelDataV2& source,
        ModelDataV2& destination,
        const ModelExtractionOptions& options);
    
    void ExtractHierarchy(
        const SceneNode* sourceNode,
        SceneNode* destNode,
        const std::vector<size_t>& meshIndices,
        const ModelExtractionOptions& options);
    
    void RemoveUnusedResources(
        ModelDataV2& model,
        const ModelExtractionOptions& options);
    
    void ApplyTransformations(
        ModelDataV2& model,
        const ModelExtractionOptions& options);
    
    bool MatchesPattern(
        const std::string& name,
        const std::string& pattern,
        bool caseSensitive);
};

// Convenience functions
inline std::unique_ptr<ModelDataV2> ExtractModelByName(
    const std::filesystem::path& file,
    const std::string& modelName,
    const ModelExtractionOptions& options = {}) {
    ModelExtractor extractor;
    return extractor.ExtractModel(file, modelName, options);
}

inline std::map<std::string, std::unique_ptr<ModelDataV2>> ExtractModelsByPattern(
    const std::filesystem::path& file,
    const std::string& pattern,
    const ModelExtractionOptions& options = {}) {
    ModelExtractor extractor;
    ModelSelectionCriteria criteria;
    criteria.namePatterns.push_back(pattern);
    return extractor.ExtractModels(file, criteria, options);
}

// Model query utilities
class ModelQuery {
public:
    // Find models by name pattern
    static std::vector<ModelInfo> FindByName(
        const std::vector<ModelInfo>& models,
        const std::string& pattern,
        bool caseSensitive = false);
    
    // Find models with animation
    static std::vector<ModelInfo> FindAnimated(
        const std::vector<ModelInfo>& models);
    
    // Find models by size
    static std::vector<ModelInfo> FindBySize(
        const std::vector<ModelInfo>& models,
        size_t minVertices,
        size_t maxVertices = std::numeric_limits<size_t>::max());
    
    // Find models by hierarchy
    static std::vector<ModelInfo> FindChildren(
        const std::vector<ModelInfo>& models,
        const std::string& parentName);
    
    // Sort models
    static void SortByName(std::vector<ModelInfo>& models);
    static void SortBySize(std::vector<ModelInfo>& models);
    static void SortByHierarchy(std::vector<ModelInfo>& models);
};