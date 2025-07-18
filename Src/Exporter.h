#pragma once
#include <string>
#include "SkinMesh.h"
#include "Skeleton.h"

class Exporter {
public:
  static bool ExportMesh(const std::string& filename, const SkinMesh& mesh);
  static bool ExportSkeleton(const std::string& filename, const Skeleton& skel);
  static bool ExportAnimation(const std::string& filename, const Skeleton& skel);
  static bool ExportGltf(const std::string& filename,
    const SkinMesh& mesh,
    const Skeleton& skel);
};