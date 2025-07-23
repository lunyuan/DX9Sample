# Model Loader API Analysis

## Current Architecture

### 1. IModelLoader Interface
```cpp
struct IModelLoader {
  virtual ~IModelLoader() = default;
  
  // Load all models from file
  [[nodiscard]] virtual std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const = 0;
  
  // Get model names without loading
  [[nodiscard]] virtual std::vector<std::string>
    GetModelNames(const std::filesystem::path& file) const = 0;
};
```

### 2. Current Implementations

#### XModelLoader (.x files)
- Uses DirectX native D3DXLoadMeshFromX
- Supports materials and textures
- Limited animation support through D3DXAnimationController

#### FbxLoader (.fbx files)
- Uses Autodesk FBX SDK
- Full skeletal animation support
- Material and texture extraction
- Hierarchical node structure

#### GltfLoader (.gltf/.glb files)
- Uses tiny_gltf library
- **NOT implementing IModelLoader interface** (design inconsistency)
- Static methods instead of virtual interface
- Supports PBR materials
- Full animation support

## Analysis of Each Format

### DirectX .X Format
**Pros:**
- Native DirectX support
- Simple to load
- Good for legacy content

**Cons:**
- Deprecated format
- Limited features
- Poor tooling support

### FBX Format
**Pros:**
- Industry standard
- Rich feature set
- Excellent tool support
- Hierarchical scene graph

**Cons:**
- Large SDK dependency
- Proprietary format
- Complex API

### glTF 2.0 Format
**Pros:**
- Open standard
- Modern PBR workflow
- Efficient binary format (GLB)
- Web-friendly
- Growing tool support

**Cons:**
- Requires conversion for DirectX
- Additional parsing complexity

## Common Data Requirements

### Mesh Data
- Vertex positions
- Normals
- UV coordinates
- Vertex colors (optional)
- Tangents/Bitangents (for normal mapping)

### Skinning Data
- Bone indices (up to 4 per vertex)
- Bone weights (normalized)
- Bind pose matrices

### Material Data
- Diffuse color/texture
- Specular properties
- Normal maps
- Metallic/Roughness (PBR)
- Emissive

### Animation Data
- Skeletal hierarchy
- Bone transforms
- Animation clips
- Keyframe data

### Scene Data
- Node hierarchy
- Transform matrices
- Multiple meshes
- LOD levels

## Proposed Refactored Architecture

### 1. Enhanced IModelLoader
```cpp
struct ModelLoadOptions {
  bool loadTextures = true;
  bool loadAnimations = true;
  bool generateTangents = false;
  bool optimizeMeshes = false;
  float scaleFactor = 1.0f;
};

struct IModelLoader {
  // Single model loading
  virtual std::unique_ptr<ModelData> LoadModel(
    const std::filesystem::path& file,
    const std::string& modelName,
    IDirect3DDevice9* device,
    const ModelLoadOptions& options = {}) = 0;
  
  // All models loading
  virtual std::map<std::string, std::unique_ptr<ModelData>> LoadAll(
    const std::filesystem::path& file,
    IDirect3DDevice9* device,
    const ModelLoadOptions& options = {}) = 0;
  
  // Metadata queries
  virtual std::vector<std::string> GetModelNames(
    const std::filesystem::path& file) const = 0;
  
  virtual bool CanLoad(
    const std::filesystem::path& file) const = 0;
  
  // Format-specific capabilities
  virtual ModelCapabilities GetCapabilities() const = 0;
};
```

### 2. Model Saver Interface
```cpp
struct ModelSaveOptions {
  bool embedTextures = false;
  bool compressData = true;
  bool includeAnimations = true;
  std::string authorName;
  std::string copyright;
};

struct IModelSaver {
  // Save single model
  virtual bool SaveModel(
    const ModelData& model,
    const std::filesystem::path& file,
    const ModelSaveOptions& options = {}) = 0;
  
  // Save multiple models
  virtual bool SaveAll(
    const std::map<std::string, ModelData>& models,
    const std::filesystem::path& file,
    const ModelSaveOptions& options = {}) = 0;
  
  // Check if format supports features
  virtual bool CanSave(const ModelData& model) const = 0;
  
  // Get supported extensions
  virtual std::vector<std::string> GetSupportedExtensions() const = 0;
};
```

### 3. Enhanced ModelData
```cpp
struct ModelMetadata {
  std::string name;
  std::string author;
  std::string copyright;
  std::string sourceFile;
  float unitScale = 1.0f;
  std::string upAxis = "Y";
};

struct ModelData {
  // Core data
  SkinMesh mesh;
  Skeleton skeleton;
  std::vector<Animation> animations;
  
  // Metadata
  ModelMetadata metadata;
  
  // Scene graph (optional)
  std::unique_ptr<SceneNode> rootNode;
  
  // LOD levels
  std::vector<std::unique_ptr<SkinMesh>> lodMeshes;
  
  // Bounding volumes
  BoundingBox boundingBox;
  BoundingSphere boundingSphere;
};
```

## Implementation Priorities

### Phase 1: Refactor Existing
1. Update IModelLoader interface
2. Refactor XModelLoader to new interface
3. Refactor FbxLoader to new interface
4. Convert GltfLoader to proper implementation

### Phase 2: Implement Savers
1. Implement GltfSaver (most modern)
2. Implement FbxSaver (industry standard)
3. Consider XSaver for legacy support

### Phase 3: Advanced Features
1. Model optimization
2. Format conversion
3. Batch processing
4. Async loading