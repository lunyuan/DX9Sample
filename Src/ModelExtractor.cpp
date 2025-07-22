#include "ModelExtractor.h"
#include <algorithm>
#include <iostream>
#include <cctype>

std::vector<ModelInfo> ModelExtractor::GetModelInfo(
    const std::filesystem::path& file,
    IModelLoaderV2* loader) {
    
    std::vector<ModelInfo> infos;
    
    // Auto-detect loader if not provided
    std::unique_ptr<IModelLoaderV2> autoLoader;
    if (!loader) {
        std::string ext = file.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        // TODO: Create appropriate loader based on extension
        // For now, return empty
        return infos;
    }
    
    // Get model names without loading
    auto modelNames = loader->GetModelNames(file);
    
    // For detailed info, we need to load the models
    // This is a trade-off between performance and information detail
    ModelLoadOptions loadOptions;
    loadOptions.loadTextures = false; // Don't load textures for info
    loadOptions.loadAnimations = true; // But do load animation info
    
    auto models = loader->LoadAll(file, nullptr, loadOptions);
    
    size_t index = 0;
    for (const auto& [name, model] : models) {
        ModelInfo info;
        info.name = name;
        info.index = index++;
        
        if (model) {
            // Mesh statistics
            for (const auto& mesh : model->meshes) {
                if (mesh) {
                    info.vertexCount += mesh->vertices.size();
                    info.triangleCount += mesh->indices.size() / 3;
                    info.materialCount += mesh->materials.size();
                }
            }
            
            // Animation info
            info.hasAnimation = !model->animations.empty();
            info.hasSkeleton = !model->skeleton.joints.empty();
            info.boneCount = model->skeleton.joints.size();
            
            for (const auto& anim : model->animations) {
                info.animationClips.push_back(anim.name);
            }
            
            // Bounds
            info.bounds = model->boundingBox;
            
            // Hierarchy info
            if (model->rootNode) {
                // TODO: Extract hierarchy information
            }
            
            // Metadata
            info.metadata = model->metadata.customProperties;
        }
        
        infos.push_back(info);
    }
    
    return infos;
}

std::unique_ptr<ModelDataV2> ModelExtractor::ExtractModel(
    const std::filesystem::path& file,
    const std::string& modelName,
    const ModelExtractionOptions& options,
    IModelLoaderV2* loader) {
    
    if (!loader) {
        // TODO: Auto-create loader
        return nullptr;
    }
    
    // Load the specific model
    auto model = loader->LoadModel(file, modelName, nullptr);
    if (!model) {
        return nullptr;
    }
    
    // Apply extraction options
    ApplyTransformations(*model, options);
    RemoveUnusedResources(*model, options);
    
    return model;
}

std::unique_ptr<ModelDataV2> ModelExtractor::ExtractModelByIndex(
    const std::filesystem::path& file,
    size_t index,
    const ModelExtractionOptions& options,
    IModelLoaderV2* loader) {
    
    if (!loader) {
        return nullptr;
    }
    
    // Get model names
    auto modelNames = loader->GetModelNames(file);
    if (index >= modelNames.size()) {
        return nullptr;
    }
    
    return ExtractModel(file, modelNames[index], options, loader);
}

std::map<std::string, std::unique_ptr<ModelDataV2>> ModelExtractor::ExtractModels(
    const std::filesystem::path& file,
    const ModelSelectionCriteria& criteria,
    const ModelExtractionOptions& options,
    IModelLoaderV2* loader) {
    
    std::map<std::string, std::unique_ptr<ModelDataV2>> result;
    
    if (!loader) {
        return result;
    }
    
    // Get model info
    auto infos = GetModelInfo(file, loader);
    
    // Filter by criteria
    std::vector<std::string> selectedNames;
    for (const auto& info : infos) {
        if (MatchesCriteria(info, criteria)) {
            selectedNames.push_back(info.name);
        }
    }
    
    // Load selected models
    for (const auto& name : selectedNames) {
        auto model = ExtractModel(file, name, options, loader);
        if (model) {
            std::string finalName = GenerateModelName(name, result.size(), options);
            result[finalName] = std::move(model);
        }
    }
    
    return result;
}

std::map<std::string, std::unique_ptr<ModelDataV2>> ModelExtractor::ExtractAllModels(
    const std::filesystem::path& file,
    const ModelExtractionOptions& options,
    IModelLoaderV2* loader) {
    
    if (!loader) {
        return {};
    }
    
    auto models = loader->LoadAll(file, nullptr);
    
    // Apply extraction options to each model
    for (auto& [name, model] : models) {
        if (model) {
            ApplyTransformations(*model, options);
            RemoveUnusedResources(*model, options);
        }
    }
    
    return models;
}

bool ModelExtractor::SplitModelsToFiles(
    const std::filesystem::path& inputFile,
    const std::filesystem::path& outputDirectory,
    const ModelSelectionCriteria& criteria,
    const ModelExtractionOptions& options,
    IModelLoaderV2* loader,
    IModelSaver* saver) {
    
    if (!loader || !saver) {
        return false;
    }
    
    // Create output directory if needed
    std::filesystem::create_directories(outputDirectory);
    
    // Extract models
    auto models = ExtractModels(inputFile, criteria, options, loader);
    
    // Save each model
    bool allSuccess = true;
    for (const auto& [name, model] : models) {
        if (!model) continue;
        
        // Generate output filename
        std::filesystem::path outputFile = outputDirectory / (name + ".glb");
        
        // Save model
        ModelSaveOptions saveOptions;
        saveOptions.embedTextures = options.includeTextures;
        saveOptions.includeAnimations = options.includeAnimations;
        
        auto result = saver->SaveModel(*model, outputFile, saveOptions);
        if (!result.success) {
            std::cerr << "Failed to save " << name << ": " << result.errorMessage << std::endl;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::unique_ptr<ModelDataV2> ModelExtractor::MergeModels(
    const std::vector<std::unique_ptr<ModelDataV2>>& models,
    const std::string& mergedName) {
    
    if (models.empty()) {
        return nullptr;
    }
    
    auto merged = std::make_unique<ModelDataV2>();
    merged->metadata.name = mergedName;
    
    // Create root node for merged model
    merged->rootNode = std::make_unique<SceneNode>();
    merged->rootNode->name = mergedName;
    
    size_t meshOffset = 0;
    
    for (size_t i = 0; i < models.size(); ++i) {
        const auto& model = models[i];
        if (!model) continue;
        
        // Create child node for this model
        auto childNode = std::make_unique<SceneNode>();
        childNode->name = model->metadata.name.empty() ? 
            ("Model_" + std::to_string(i)) : model->metadata.name;
        
        // Copy meshes
        for (const auto& mesh : model->meshes) {
            if (mesh) {
                merged->meshes.push_back(std::make_unique<SkinMesh>(*mesh));
                childNode->meshIndices.push_back(meshOffset++);
            }
        }
        
        // Merge skeletons (if compatible)
        // TODO: Implement skeleton merging
        
        // Merge animations
        for (const auto& anim : model->animations) {
            merged->animations.push_back(anim);
        }
        
        merged->rootNode->children.push_back(std::move(childNode));
    }
    
    // Update bounds and statistics
    merged->CalculateBoundingVolumes();
    merged->UpdateStatistics();
    
    return merged;
}

std::vector<std::string> ModelExtractor::PreviewExtraction(
    const std::filesystem::path& file,
    const ModelSelectionCriteria& criteria,
    IModelLoaderV2* loader) {
    
    std::vector<std::string> result;
    
    if (!loader) {
        return result;
    }
    
    auto infos = GetModelInfo(file, loader);
    
    for (const auto& info : infos) {
        if (MatchesCriteria(info, criteria)) {
            result.push_back(info.name);
        }
    }
    
    return result;
}

bool ModelExtractor::MatchesCriteria(
    const ModelInfo& info,
    const ModelSelectionCriteria& criteria) {
    
    bool matches = false;
    
    // Check exact names
    if (!criteria.modelNames.empty()) {
        auto it = std::find(criteria.modelNames.begin(), 
                           criteria.modelNames.end(), 
                           info.name);
        if (it != criteria.modelNames.end()) {
            matches = true;
        }
    }
    
    // Check name patterns
    for (const auto& pattern : criteria.namePatterns) {
        try {
            std::regex re(pattern, criteria.caseSensitive ? 
                std::regex::ECMAScript : 
                std::regex::icase);
            if (std::regex_match(info.name, re)) {
                matches = true;
                break;
            }
        } catch (...) {
            // Invalid regex
        }
    }
    
    // Check indices
    if (!criteria.modelIndices.empty()) {
        auto it = std::find(criteria.modelIndices.begin(),
                           criteria.modelIndices.end(),
                           info.index);
        if (it != criteria.modelIndices.end()) {
            matches = true;
        }
    }
    
    // If OR mode and already matched, return true
    if (criteria.combineMode == ModelSelectionCriteria::CombineMode::OR && matches) {
        return true;
    }
    
    // Check mesh properties
    bool meetsProperties = true;
    if (info.vertexCount < criteria.minVertexCount ||
        info.vertexCount > criteria.maxVertexCount) {
        meetsProperties = false;
    }
    
    if (info.triangleCount < criteria.minTriangleCount ||
        info.triangleCount > criteria.maxTriangleCount) {
        meetsProperties = false;
    }
    
    if (criteria.mustHaveAnimation && !info.hasAnimation) {
        meetsProperties = false;
    }
    
    if (criteria.mustHaveSkeleton && !info.hasSkeleton) {
        meetsProperties = false;
    }
    
    // Check hierarchy
    if (!criteria.parentNodeName.empty() && 
        info.parentNodeName != criteria.parentNodeName) {
        meetsProperties = false;
    }
    
    // Combine results based on mode
    if (criteria.combineMode == ModelSelectionCriteria::CombineMode::AND) {
        return matches && meetsProperties;
    } else {
        return matches || meetsProperties;
    }
}

std::string ModelExtractor::GenerateModelName(
    const std::string& originalName,
    size_t index,
    const ModelExtractionOptions& options) {
    
    std::string result = options.namingPattern;
    
    // Replace placeholders
    size_t pos = result.find("{original}");
    if (pos != std::string::npos) {
        result.replace(pos, 10, originalName);
    }
    
    pos = result.find("{index}");
    if (pos != std::string::npos) {
        result.replace(pos, 7, std::to_string(index));
    }
    
    // Add prefix and suffix
    result = options.namePrefix + result + options.nameSuffix;
    
    return result;
}

void ModelExtractor::ApplyTransformations(
    ModelDataV2& model,
    const ModelExtractionOptions& options) {
    
    if (options.applyTransforms) {
        // TODO: Bake transforms into vertex positions
    }
    
    if (options.centerModels) {
        // Calculate center
        model.CalculateBoundingVolumes();
        auto center = model.boundingBox.GetCenter();
        
        // Translate all vertices
        for (auto& mesh : model.meshes) {
            if (!mesh) continue;
            
            for (auto& vertex : mesh->vertices) {
                vertex.pos.x -= center.x;
                vertex.pos.y -= center.y;
                vertex.pos.z -= center.z;
            }
        }
    }
    
    if (options.normalizeScale) {
        // Calculate scale factor
        model.CalculateBoundingVolumes();
        auto size = model.boundingBox.GetSize();
        float maxDim = std::max({size.x, size.y, size.z});
        
        if (maxDim > 0) {
            float scale = 1.0f / maxDim;
            
            // Scale all vertices
            for (auto& mesh : model.meshes) {
                if (!mesh) continue;
                
                for (auto& vertex : mesh->vertices) {
                    vertex.pos.x *= scale;
                    vertex.pos.y *= scale;
                    vertex.pos.z *= scale;
                }
            }
        }
    }
}

void ModelExtractor::RemoveUnusedResources(
    ModelDataV2& model,
    const ModelExtractionOptions& options) {
    
    if (options.removeUnusedBones) {
        // TODO: Find and remove bones with no vertex weights
    }
    
    if (options.removeUnusedMaterials) {
        // TODO: Find and remove materials not referenced by any mesh
    }
    
    if (options.optimizeMeshes) {
        model.OptimizeMeshes();
    }
}

// ModelQuery implementations
std::vector<ModelInfo> ModelQuery::FindByName(
    const std::vector<ModelInfo>& models,
    const std::string& pattern,
    bool caseSensitive) {
    
    std::vector<ModelInfo> result;
    
    try {
        std::regex re(pattern, caseSensitive ? 
            std::regex::ECMAScript : 
            std::regex::icase);
            
        for (const auto& model : models) {
            if (std::regex_search(model.name, re)) {
                result.push_back(model);
            }
        }
    } catch (...) {
        // Invalid regex - do simple substring search
        std::string searchPattern = pattern;
        if (!caseSensitive) {
            std::transform(searchPattern.begin(), searchPattern.end(), 
                         searchPattern.begin(), ::tolower);
        }
        
        for (const auto& model : models) {
            std::string name = model.name;
            if (!caseSensitive) {
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            }
            
            if (name.find(searchPattern) != std::string::npos) {
                result.push_back(model);
            }
        }
    }
    
    return result;
}

std::vector<ModelInfo> ModelQuery::FindAnimated(
    const std::vector<ModelInfo>& models) {
    
    std::vector<ModelInfo> result;
    
    for (const auto& model : models) {
        if (model.hasAnimation) {
            result.push_back(model);
        }
    }
    
    return result;
}

std::vector<ModelInfo> ModelQuery::FindBySize(
    const std::vector<ModelInfo>& models,
    size_t minVertices,
    size_t maxVertices) {
    
    std::vector<ModelInfo> result;
    
    for (const auto& model : models) {
        if (model.vertexCount >= minVertices && 
            model.vertexCount <= maxVertices) {
            result.push_back(model);
        }
    }
    
    return result;
}

void ModelQuery::SortByName(std::vector<ModelInfo>& models) {
    std::sort(models.begin(), models.end(),
        [](const ModelInfo& a, const ModelInfo& b) {
            return a.name < b.name;
        });
}

void ModelQuery::SortBySize(std::vector<ModelInfo>& models) {
    std::sort(models.begin(), models.end(),
        [](const ModelInfo& a, const ModelInfo& b) {
            return a.vertexCount < b.vertexCount;
        });
}