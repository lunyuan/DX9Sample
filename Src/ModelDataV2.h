#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <DirectXMath.h>
#include "SkinMesh.h"
#include "Skeleton.h"

// Model metadata
struct ModelMetadata {
    std::string name;
    std::string author;
    std::string copyright;
    std::string description;
    std::string sourceFile;
    std::string applicationName;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point modificationTime;
    
    // Unit and coordinate system info
    float unitScale = 1.0f;        // Units per meter
    std::string upAxis = "Y";      // Y-up (default), Z-up
    std::string forwardAxis = "Z"; // Z-forward (default), -Z-forward
    
    // Custom properties
    std::map<std::string, std::string> customProperties;
};

// Bounding volume structures
struct BoundingBox {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;
    
    DirectX::XMFLOAT3 GetCenter() const {
        return DirectX::XMFLOAT3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }
    
    DirectX::XMFLOAT3 GetSize() const {
        return DirectX::XMFLOAT3(
            max.x - min.x,
            max.y - min.y,
            max.z - min.z
        );
    }
};

struct BoundingSphere {
    DirectX::XMFLOAT3 center;
    float radius;
};

// Scene node for hierarchical models
struct SceneNode {
    std::string name;
    DirectX::XMFLOAT4X4 transform;
    std::vector<std::unique_ptr<SceneNode>> children;
    
    // Node can reference multiple meshes
    std::vector<size_t> meshIndices;
    
    // Node-specific properties
    bool visible = true;
    std::map<std::string, std::string> properties;
};

// Animation clip information
struct AnimationClip {
    std::string name;
    float duration;      // In seconds
    float ticksPerSecond = 30.0f;
    bool looping = true;
    
    // Bone animation tracks
    std::map<std::string, AnimationTrack> boneTracks;
    
    // Property animation tracks (for future use)
    std::map<std::string, PropertyAnimationTrack> propertyTracks;
};

// Property animation for non-skeletal animation
struct PropertyAnimationTrack {
    enum class PropertyType {
        Translation,
        Rotation,
        Scale,
        Visibility,
        Color,
        Custom
    };
    
    PropertyType type;
    std::string targetPath; // Node path
    std::vector<float> times;
    std::vector<DirectX::XMFLOAT4> values;
};

// Enhanced model data structure
struct ModelDataV2 {
    // Core mesh data
    std::vector<std::unique_ptr<SkinMesh>> meshes;
    
    // Skeletal data
    Skeleton skeleton;
    
    // Animation data
    std::vector<AnimationClip> animations;
    size_t defaultAnimationIndex = 0;
    
    // Scene hierarchy (optional)
    std::unique_ptr<SceneNode> rootNode;
    
    // Level of Detail meshes
    struct LODLevel {
        float distance;  // Switch distance
        std::vector<size_t> meshIndices; // Indices into meshes array
    };
    std::vector<LODLevel> lodLevels;
    
    // Bounding volumes
    BoundingBox boundingBox;
    BoundingSphere boundingSphere;
    
    // Metadata
    ModelMetadata metadata;
    
    // Material library
    std::map<std::string, Material> materials;
    
    // Texture paths (for external textures)
    std::map<std::string, std::filesystem::path> texturePaths;
    
    // Statistics
    struct Statistics {
        size_t vertexCount = 0;
        size_t triangleCount = 0;
        size_t boneCount = 0;
        size_t materialCount = 0;
        size_t textureCount = 0;
        size_t animationCount = 0;
        size_t totalMemoryUsage = 0;
    };
    Statistics stats;
    
    // Helper methods
    void CalculateBoundingVolumes();
    void UpdateStatistics();
    void OptimizeMeshes(float vertexWeldThreshold = 0.0001f);
    std::unique_ptr<ModelDataV2> CreateLOD(float reductionFactor) const;
};

// Backwards compatibility typedef
using ModelData = ModelDataV2;