#pragma once
#include "IModelSaver.h"
#include <fbxsdk.h>

class FbxSaver : public IModelSaver {
public:
    FbxSaver();
    ~FbxSaver();

    // IModelSaver interface
    [[nodiscard]] ModelSaveResult SaveModel(
        const ModelData& model,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) override;
    
    [[nodiscard]] ModelSaveResult SaveAll(
        const std::map<std::string, ModelData>& models,
        const std::filesystem::path& file,
        const ModelSaveOptions& options = {}) override;
    
    [[nodiscard]] bool CanSave(const ModelData& model) const override;
    
    [[nodiscard]] bool SupportsMultipleModels() const override { return true; }
    
    [[nodiscard]] ModelSaveCapabilities GetCapabilities() const override;
    
    [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override {
        return { ".fbx" };
    }
    
    [[nodiscard]] bool ValidateOptions(const ModelSaveOptions& options) const override;
    
    [[nodiscard]] size_t EstimateFileSize(
        const ModelData& model,
        const ModelSaveOptions& options) const override;

private:
    FbxManager* fbxManager_;
    
    // Helper methods
    FbxNode* CreateMeshNode(FbxScene* scene, const std::string& name, const ModelData& model);
    FbxMesh* CreateFbxMesh(FbxScene* scene, const SkinMesh& mesh);
    void ApplyMaterials(FbxNode* node, FbxMesh* fbxMesh, const SkinMesh& mesh, const std::filesystem::path& basePath);
    bool ExportScene(FbxScene* scene, const std::filesystem::path& file, const ModelSaveOptions& options);
};

// Factory function
std::unique_ptr<IModelSaver> CreateFbxSaver();