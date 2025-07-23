# Model Extraction Examples

## Overview
The model extraction system allows you to extract specific models from files containing multiple models, split them into separate files, or merge multiple models together.

## Basic Usage

### 1. Extract Single Model by Name
```cpp
#include "ModelExtractor.h"

// Extract a specific model by name
ModelExtractor extractor;
auto model = extractor.ExtractModel("scene.fbx", "Character_Hero");

// Or use the convenience function
auto model = ExtractModelByName("scene.fbx", "Character_Hero");
```

### 2. Extract Multiple Models by Pattern
```cpp
// Extract all character models
ModelSelectionCriteria criteria;
criteria.namePatterns.push_back("Character_.*");  // Regex pattern

ModelExtractor extractor;
auto models = extractor.ExtractModels("scene.fbx", criteria);

// Or use convenience function
auto models = ExtractModelsByPattern("scene.fbx", "Character_.*");
```

### 3. Extract Models by Properties
```cpp
// Extract only animated models with more than 1000 vertices
ModelSelectionCriteria criteria;
criteria.minVertexCount = 1000;
criteria.mustHaveAnimation = true;

ModelExtractor extractor;
auto models = extractor.ExtractModels("scene.fbx", criteria);
```

### 4. Split Models to Separate Files
```cpp
// Split all character models into separate files
ModelSelectionCriteria criteria;
criteria.namePatterns.push_back("Character_.*");

ModelExtractionOptions options;
options.includeAnimations = true;
options.centerModels = true;
options.namingPattern = "extracted_{original}";

ModelExtractor extractor;
bool success = extractor.SplitModelsToFiles(
    "scene.fbx",                    // Input file
    "output/characters/",           // Output directory
    criteria,                       // Selection criteria
    options,                        // Extraction options
    CreateFbxLoaderV2().get(),     // Loader
    CreateGltfSaver().get()        // Saver
);
```

## Advanced Usage

### 1. Complex Selection Criteria
```cpp
// Select specific models with multiple criteria
ModelSelectionCriteria criteria;

// By exact names
criteria.modelNames = {"Hero", "Villain", "NPC_01"};

// By patterns
criteria.namePatterns = {"Weapon_.*", "Armor_.*"};

// By properties
criteria.minVertexCount = 500;
criteria.maxVertexCount = 10000;
criteria.mustHaveSkeleton = true;

// By hierarchy
criteria.parentNodeName = "Characters";
criteria.maxDepth = 2;

// Combine with OR logic (match any criteria)
criteria.combineMode = ModelSelectionCriteria::CombineMode::OR;
```

### 2. Extraction Options
```cpp
ModelExtractionOptions options;

// Resource handling
options.includeAnimations = true;
options.includeMaterials = true;
options.includeTextures = true;
options.preserveHierarchy = true;

// Transformations
options.applyTransforms = true;      // Bake transforms
options.centerModels = true;         // Center at origin
options.normalizeScale = true;       // Scale to unit size

// Optimization
options.removeUnusedBones = true;
options.removeUnusedMaterials = true;
options.optimizeMeshes = true;

// Naming
options.namingPattern = "{parent}_{original}_{index}";
options.namePrefix = "extracted_";
options.nameSuffix = "_v2";
```

### 3. Query and Filter Models
```cpp
// Get model information
ModelExtractor extractor;
auto infos = extractor.GetModelInfo("scene.fbx");

// Query models
auto characters = ModelQuery::FindByName(infos, "Character_.*");
auto animated = ModelQuery::FindAnimated(infos);
auto largeModels = ModelQuery::FindBySize(infos, 10000); // 10k+ vertices

// Sort models
ModelQuery::SortByName(characters);
ModelQuery::SortBySize(largeModels);

// Preview extraction
auto preview = extractor.PreviewExtraction("scene.fbx", criteria);
for (const auto& name : preview) {
    std::cout << "Will extract: " << name << std::endl;
}
```

### 4. Merge Models
```cpp
// Load individual models
std::vector<std::unique_ptr<ModelDataV2>> models;
models.push_back(ExtractModelByName("weapons.fbx", "Sword"));
models.push_back(ExtractModelByName("armor.fbx", "Shield"));
models.push_back(ExtractModelByName("character.fbx", "Knight"));

// Merge into single model
ModelExtractor extractor;
auto merged = extractor.MergeModels(models, "Knight_Complete");

// Save merged model
auto saver = CreateGltfSaver();
saver->SaveModel(*merged, "knight_complete.glb");
```

## Practical Examples

### Example 1: Extract Characters from Game Scene
```cpp
void ExtractCharactersFromScene(const std::string& sceneFile) {
    ModelExtractor extractor;
    
    // Define what we're looking for
    ModelSelectionCriteria criteria;
    criteria.namePatterns.push_back("Character_.*");
    criteria.namePatterns.push_back("NPC_.*");
    criteria.mustHaveSkeleton = true;
    criteria.minVertexCount = 1000;
    
    // Set extraction options
    ModelExtractionOptions options;
    options.includeAnimations = true;
    options.centerModels = true;
    options.removeUnusedBones = true;
    options.namingPattern = "char_{original}";
    
    // Extract and save
    extractor.SplitModelsToFiles(
        sceneFile,
        "extracted/characters/",
        criteria,
        options,
        CreateFbxLoaderV2().get(),
        CreateGltfSaver().get()
    );
}
```

### Example 2: Extract LOD Models
```cpp
void ExtractLODModels(const std::string& modelFile) {
    ModelExtractor extractor;
    
    // Extract LOD0 (high detail)
    ModelSelectionCriteria lod0;
    lod0.namePatterns.push_back(".*_LOD0");
    lod0.minVertexCount = 5000;
    
    // Extract LOD1 (medium detail)
    ModelSelectionCriteria lod1;
    lod1.namePatterns.push_back(".*_LOD1");
    lod1.minVertexCount = 1000;
    lod1.maxVertexCount = 5000;
    
    // Extract LOD2 (low detail)
    ModelSelectionCriteria lod2;
    lod2.namePatterns.push_back(".*_LOD2");
    lod2.maxVertexCount = 1000;
    
    // Extract each LOD level
    auto loader = CreateFbxLoaderV2();
    auto saver = CreateGltfSaver();
    
    extractor.SplitModelsToFiles(modelFile, "lods/high/", lod0, {}, 
                                loader.get(), saver.get());
    extractor.SplitModelsToFiles(modelFile, "lods/medium/", lod1, {}, 
                                loader.get(), saver.get());
    extractor.SplitModelsToFiles(modelFile, "lods/low/", lod2, {}, 
                                loader.get(), saver.get());
}
```

### Example 3: Extract and Optimize for Mobile
```cpp
void ExtractForMobile(const std::string& sourceFile) {
    ModelExtractor extractor;
    
    // Select appropriate models
    ModelSelectionCriteria criteria;
    criteria.maxVertexCount = 5000;  // Mobile-friendly poly count
    criteria.excludeTags = {"HighDetail", "PCOnly"};
    
    // Optimization options
    ModelExtractionOptions options;
    options.optimizeMeshes = true;
    options.removeUnusedBones = true;
    options.removeUnusedMaterials = true;
    options.normalizeScale = true;
    options.includeTextures = false;  // Handle textures separately
    
    // Extract and save
    auto models = extractor.ExtractModels(sourceFile, criteria, options);
    
    // Further process each model
    for (auto& [name, model] : models) {
        // Reduce texture sizes, compress, etc.
        ProcessForMobile(*model);
        
        // Save optimized model
        SaveModelForMobile(*model, name);
    }
}
```

## Command Line Usage

### Basic extraction
```bash
ModelExtractor.exe -i scene.fbx -o output/ -pattern "Character_.*"
```

### With options
```bash
ModelExtractor.exe -i scene.fbx -o output/ \
    -pattern ".*" \
    -min-vertices 1000 \
    -must-have-animation \
    -center-models \
    -optimize
```

### Split by criteria
```bash
ModelExtractor.exe -i scene.fbx -o output/ \
    -split-by-parent \
    -format glb \
    -embed-textures
```

## Best Practices

1. **Preview Before Extraction**
   - Use `PreviewExtraction()` to verify selection
   - Check model info before processing

2. **Optimize Extraction**
   - Don't load textures if not needed
   - Use appropriate selection criteria
   - Process in batches for large files

3. **Handle Resources Properly**
   - Decide on texture handling (embed vs external)
   - Consider shared resources
   - Clean up unused data

4. **Naming Conventions**
   - Use clear naming patterns
   - Include version or date in output
   - Maintain hierarchy information

5. **Error Handling**
   - Check loader/saver capabilities
   - Validate options before processing
   - Handle missing models gracefully