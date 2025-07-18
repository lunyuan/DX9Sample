#include "FbxLoader.h"
#include <iostream>
#include <functional>
#include <DirectXMath.h>
using namespace fbxsdk;

FbxLoader::FbxLoader() {
  mgr = FbxManager::Create();
  FbxIOSettings* ios = FbxIOSettings::Create(mgr, IOSROOT);
  mgr->SetIOSettings(ios);
}

FbxLoader::~FbxLoader() {
  if (scene) scene->Destroy(true);
  if (mgr) mgr->Destroy();
}

bool FbxLoader::Load(const std::string& path) {
  FbxImporter* importer = FbxImporter::Create(mgr, "");
  if (!importer->Initialize(path.c_str(), -1, mgr->GetIOSettings())) {
    std::cerr << "Failed to initialize FBX importer for " << path << std::endl;
    importer->Destroy();
    return false;
  }
  scene = FbxScene::Create(mgr, "scene");
  importer->Import(scene);
  importer->Destroy();
  return true;
}

void FbxLoader::Convert(SkinMesh& outMesh, Skeleton& outSkel) {
  FbxNode* root = scene->GetRootNode();
  if (!root) return;
  // Gather skeleton joints
  std::vector<FbxNode*> bones;
  std::function<void(FbxNode*)> gather = [&](FbxNode* node) {
    if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
      bones.push_back(node);
    for (int i = 0; i < node->GetChildCount(); ++i)
      gather(node->GetChild(i));
    };
  gather(root);
  size_t jcount = bones.size();
  outSkel.joints.resize(jcount);
  for (size_t i = 0; i < jcount; ++i) {
    FbxNode* node = bones[i];
    outSkel.joints[i].name = node->GetName();
    outSkel.joints[i].parentIndex = -1;
    for (size_t p = 0; p < jcount; ++p) {
      if (bones[p] == node->GetParent()) {
        outSkel.joints[i].parentIndex = static_cast<int>(p);
        break;
      }
    }
    // Bind-pose inverse
    FbxAMatrix global = node->EvaluateGlobalTransform();
    DirectX::XMFLOAT4X4 mat;
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c)
      mat.m[r][c] = static_cast<float>(global.Get(r, c));
    DirectX::XMMATRIX inv = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&mat));
    DirectX::XMStoreFloat4x4(&outSkel.joints[i].bindPoseInverse, inv);
  }
  // Extract mesh and skin
  int childCount = root->GetChildCount();
  for (int n = 0; n < childCount; ++n) {
    FbxNode* node = root->GetChild(n);
    if (node->GetMesh()) {
      FbxMesh* fmesh = node->GetMesh();
      int cpCount = fmesh->GetControlPointsCount();
      outMesh.vertices.resize(cpCount);
      for (int v = 0; v < cpCount; ++v) {
        FbxVector4 cp = fmesh->GetControlPointAt(v);
        outMesh.vertices[v].pos.x = static_cast<float>(cp[0]);
        outMesh.vertices[v].pos.y = static_cast<float>(cp[1]);
        outMesh.vertices[v].pos.z = static_cast<float>(cp[2]);
        memset(&outMesh.vertices[v].boneIndices, 0, 4);
        outMesh.vertices[v].weights = { 0.0f, 0.0f, 0.0f, 0.0f };
      }
      int polyCount = fmesh->GetPolygonCount();
      for (int p = 0; p < polyCount; ++p) {
        for (int v = 0; v < 3; ++v) {
          int index = fmesh->GetPolygonVertex(p, v);
          outMesh.indices.push_back(index);
        }
      }
      // Skin weights
      int deformerCount = fmesh->GetDeformerCount();
      for (int d = 0; d < deformerCount; ++d) {
        if (fmesh->GetDeformer(d)->GetDeformerType() == FbxDeformer::eSkin) {
          FbxSkin* skin = static_cast<FbxSkin*>(fmesh->GetDeformer(d));
          int clusterCount = skin->GetClusterCount();
          for (int c = 0; c < clusterCount; ++c) {
            FbxCluster* cluster = skin->GetCluster(c);
            int jointIndex = static_cast<int>(std::find(bones.begin(), bones.end(), cluster->GetLink()) - bones.begin());
            int cpIndexCount = cluster->GetControlPointIndicesCount();
            auto cpIndices = cluster->GetControlPointIndices();
            auto cpWeights = cluster->GetControlPointWeights();
            for (int i = 0; i < cpIndexCount; ++i) {
              int idx = cpIndices[i];
              float w = static_cast<float>(cpWeights[i]);
              if(outMesh.vertices[idx].weights.x == 0.0f) {
                outMesh.vertices[idx].boneIndices[0] = static_cast<uint8_t>(jointIndex);
                outMesh.vertices[idx].weights.x = w;
              }
              if (outMesh.vertices[idx].weights.y == 0.0f) {
                outMesh.vertices[idx].boneIndices[1] = static_cast<uint8_t>(jointIndex);
                outMesh.vertices[idx].weights.y = w;
              }
              if (outMesh.vertices[idx].weights.z == 0.0f) {
                outMesh.vertices[idx].boneIndices[2] = static_cast<uint8_t>(jointIndex);
                outMesh.vertices[idx].weights.z = w;
              }
              if (outMesh.vertices[idx].weights.w == 0.0f) {
                outMesh.vertices[idx].boneIndices[3] = static_cast<uint8_t>(jointIndex);
                outMesh.vertices[idx].weights.w = w;
              }
            }
          }
        }
      }
    }
  }
  // Note: animation parsing into outSkel.animations omitted here
}