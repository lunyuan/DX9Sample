#pragma once
#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <d3dx9.h>
#include "ModelData.h"

// Enhanced X model loader that properly handles multiple objects
class XModelEnhanced {
public:
    // Structure to hold information about each mesh in the X file
    struct MeshInfo {
        std::string name;
        std::string parentName;
        D3DXMATRIX transform;
        ID3DXMesh* mesh;
        std::vector<D3DMATERIAL9> materials;
        std::vector<IDirect3DTexture9*> textures;
        ID3DXSkinInfo* skinInfo;
    };
    
    // Load all meshes from X file with proper separation
    static std::map<std::string, std::shared_ptr<ModelData>> LoadWithSeparation(
        const std::filesystem::path& file,
        IDirect3DDevice9* device);
    
    // Get list of object names in the X file
    static std::vector<std::string> GetObjectNames(
        const std::filesystem::path& file,
        IDirect3DDevice9* device);
    
    // Load specific object by name
    static std::shared_ptr<ModelData> LoadObject(
        const std::filesystem::path& file,
        const std::string& objectName,
        IDirect3DDevice9* device);

private:
    // Helper to traverse frame hierarchy and collect meshes
    static void CollectMeshes(
        D3DXFRAME* frame,
        std::vector<MeshInfo>& meshes,
        const D3DXMATRIX& parentTransform,
        const std::string& parentName = "");
    
    // Convert D3DXMesh to our SkinMesh format
    static void ConvertToSkinMesh(
        const MeshInfo& meshInfo,
        SkinMesh& outMesh,
        IDirect3DDevice9* device);
    
    // Extract skeleton from frame hierarchy
    static void ExtractSkeleton(
        D3DXFRAME* frame,
        Skeleton& skeleton,
        int parentIndex = -1);
    
    // Helper to generate unique names for unnamed meshes
    static std::string GenerateUniqueName(
        const std::string& baseName,
        const std::vector<MeshInfo>& existingMeshes);
};