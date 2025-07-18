#pragma once
#include <string>
#include <functional>
#include "SkinMesh.h"
#include "Skeleton.h"

class Loader {
public:
  // ---- Callback 型別定義（如未使用可移除） ----
  using MeshCallback = std::function<void(const SkinMesh&)>;
  using SkeletonCallback = std::function<void(const Skeleton&)>;
  using AnimCallback = std::function<void(const Skeleton&)>;

  static bool LoadMesh(const std::wstring& filename, SkinMesh& mesh);
  static bool LoadSkeleton(const std::wstring& filename, Skeleton& skel);
  static bool LoadAnimation(const std::wstring& filename, Skeleton& skel);
  static bool LoadGltf(const std::string& filename,
    SkinMesh& mesh,
    Skeleton& skel);
  /** 載入 .x 格式（靜態或骨骼網格） */
  static bool LoadXFile(const std::wstring& filename, IDirect3DDevice9* dev, SkinMesh& outMesh, Skeleton& outSkel);

  // ---- 通用載入入口：依副檔名自動分發 ----

  /**
   * 根據檔名副檔名決定呼叫哪個 Load* 函式
   * @param filename 含副檔名的檔案路徑
   * @param outMesh  輸出 Mesh 資料
   * @param outSkel  輸出 Skeleton 與 Animation 資料
   */
  static bool Load(const std::wstring& filename, IDirect3DDevice9* dev, SkinMesh& outMesh, Skeleton& outSkel);
};