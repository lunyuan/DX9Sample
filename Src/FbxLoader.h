#pragma once
#include <string>
#include <fbxsdk.h>
#include "SkinMesh.h"
#include "Skeleton.h"

class FbxLoader {
public:
  FbxLoader();
  ~FbxLoader();
  bool Load(const std::string& path);
  void Convert(SkinMesh& mesh, Skeleton& skel);
private:
  FbxManager* mgr = nullptr;
  FbxScene* scene = nullptr;
};