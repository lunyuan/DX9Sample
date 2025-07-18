#pragma once
#include <string>
#include <vector>
#include "SkinMesh.h"
#include "Skeleton.h"
#include "tiny_gltf.h"


class GltfLoader {
public:
  /**
   * 從 glTF/GLB 檔案載入模型、骨架與動畫
   * @param filename glTF/GLB 檔案路徑
   * @param outMesh  輸出網格資料
   * @param outSkel  輸出骨架與動畫資料
   * @return 載入成功回傳 true
   */
  static bool Load(const std::string& filename,
    SkinMesh& outMesh,
    Skeleton& outSkel);
private:
  static void ParseMesh(const tinygltf::Model& model, SkinMesh& outMesh);
  static void ParseSkeleton(const tinygltf::Model& model, Skeleton& outSkel);
  static void ParseAnimations(const tinygltf::Model& model, Skeleton& outSkel);
};
