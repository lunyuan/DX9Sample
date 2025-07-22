#pragma once
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <fbxsdk.h>
#include "IModelLoader.h"
#include "SkinMesh.h"
#include "Skeleton.h"
#include "ModelData.h"

class FbxLoader : public IModelLoader {
public:
  FbxLoader();
  ~FbxLoader() override;
  
  // IModelLoader interface implementation
  [[nodiscard]] std::map<std::string, ModelData>
    Load(const std::filesystem::path& file, IDirect3DDevice9* device) const override;
  
  [[nodiscard]] std::vector<std::string>
    GetModelNames(const std::filesystem::path& file) const override;

private:
  // Helper methods
  bool LoadScene(const std::string& path, FbxManager* mgr, FbxScene* scene) const;
  void ConvertNode(FbxNode* node, SkinMesh& mesh, Skeleton& skel, IDirect3DDevice9* device) const;
  void ExtractMeshData(FbxMesh* fbxMesh, SkinMesh& mesh, IDirect3DDevice9* device) const;
  void ExtractMaterials(FbxNode* node, SkinMesh& mesh, IDirect3DDevice9* device) const;
  void ExtractSkeleton(FbxNode* node, Skeleton& skel) const;
  void ExtractSkinWeights(FbxMesh* fbxMesh, std::vector<std::vector<std::pair<int, float>>>& skinWeights) const;
  void ExtractAnimations(FbxScene* scene, Skeleton& skel) const;
};