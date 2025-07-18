#include "Exporter.h"
#include "tiny_gltf.h"
#include <fstream>
#include <vector>

using namespace tinygltf;

bool Exporter::ExportMesh(const std::string& filename, const SkinMesh& mesh) {
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) return false;
  uint32_t vcount = static_cast<uint32_t>(mesh.vertices.size());
  uint32_t icount = static_cast<uint32_t>(mesh.indices.size());
  ofs.write(reinterpret_cast<const char*>(&vcount), sizeof(vcount));
  ofs.write(reinterpret_cast<const char*>(&icount), sizeof(icount));
  ofs.write(reinterpret_cast<const char*>(mesh.vertices.data()), vcount * sizeof(mesh.vertices[0]));
  ofs.write(reinterpret_cast<const char*>(mesh.indices.data()), icount * sizeof(mesh.indices[0]));
  return true;
}

bool Exporter::ExportSkeleton(const std::string& filename, const Skeleton& skel) {
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) return false;
  uint32_t jcount = static_cast<uint32_t>(skel.joints.size());
  ofs.write(reinterpret_cast<const char*>(&jcount), sizeof(jcount));
  for (const auto& j : skel.joints) {
    uint32_t nameLen = static_cast<uint32_t>(j.name.size());
    ofs.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    ofs.write(j.name.c_str(), nameLen);
    ofs.write(reinterpret_cast<const char*>(&j.parentIndex), sizeof(j.parentIndex));
    ofs.write(reinterpret_cast<const char*>(&j.bindPoseInverse), sizeof(j.bindPoseInverse));
  }
  return true;
}

bool Exporter::ExportAnimation(const std::string& filename, const Skeleton& skel) {
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) return false;
  uint32_t animCount = static_cast<uint32_t>(skel.animations.size());
  ofs.write(reinterpret_cast<const char*>(&animCount), sizeof(animCount));
  for (const auto& anim : skel.animations) {
    uint32_t nameLen = static_cast<uint32_t>(anim.name.size());
    ofs.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    ofs.write(anim.name.c_str(), nameLen);
    ofs.write(reinterpret_cast<const char*>(&anim.duration), sizeof(anim.duration));
    uint32_t jointCount = static_cast<uint32_t>(anim.channels.size());
    ofs.write(reinterpret_cast<const char*>(&jointCount), sizeof(jointCount));
    for (const auto& channel : anim.channels) {
      uint32_t keyCount = static_cast<uint32_t>(channel.size());
      ofs.write(reinterpret_cast<const char*>(&keyCount), sizeof(keyCount));
      for (const auto& key : channel) {
        ofs.write(reinterpret_cast<const char*>(&key.time), sizeof(key.time));
        ofs.write(reinterpret_cast<const char*>(&key.transform), sizeof(key.transform));
      }
    }
  }
  return true;
}

bool Exporter::ExportGltf(const std::string& filename, const SkinMesh& mesh, const Skeleton& skel) {
  Model model;
  Buffer buffer;
  // Populate model with mesh/skin/animation data
  model.buffers.push_back(buffer);
  TinyGLTF gltf;
  return gltf.WriteGltfSceneToFile(&model, filename, true, true, true, false);
}