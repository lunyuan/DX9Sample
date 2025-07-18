#define NOMINMAX
#include <DirectXMath.h>
#include "GltfLoader.h"
#include <iostream>
#include <algorithm>

using namespace DirectX;

void GltfLoader::ParseMesh(const tinygltf::Model& model, SkinMesh& outMesh) {
  if (model.meshes.empty()) return;
  const auto& prim = model.meshes[0].primitives[0];
  // Positions
  const auto& posAcc = model.accessors[prim.attributes.at("POSITION")];
  const auto& posView = model.bufferViews[posAcc.bufferView];
  const auto& posBuf = model.buffers[posView.buffer];
  size_t vcount = posAcc.count;
  outMesh.vertices.resize(vcount);
  const float* posPtr = reinterpret_cast<const float*>(
    &posBuf.data[posView.byteOffset + posAcc.byteOffset]);
  for (size_t i = 0; i < vcount; ++i) {
    outMesh.vertices[i].pos.x = posPtr[3 * i + 0];
    outMesh.vertices[i].pos.y = posPtr[3 * i + 1];
    outMesh.vertices[i].pos.z = posPtr[3 * i + 2];
  }
  // Indices
  const auto& idxAcc = model.accessors[prim.indices];
  const auto& idxView = model.bufferViews[idxAcc.bufferView];
  const auto& idxBuf = model.buffers[idxView.buffer];
  const unsigned short* idxPtr = reinterpret_cast<const unsigned short*>(
    &idxBuf.data[idxView.byteOffset + idxAcc.byteOffset]);
  size_t icount = idxAcc.count;
  outMesh.indices.resize(icount);
  for (size_t i = 0; i < icount; ++i) outMesh.indices[i] = idxPtr[i];
}

void GltfLoader::ParseSkeleton(const tinygltf::Model& model, Skeleton& outSkel) {
  if (model.skins.empty()) return;
  const auto& skin = model.skins[0];
  size_t jcount = skin.joints.size();
  outSkel.joints.resize(jcount);
  // inverse bind matrices
  const auto& ibmAcc = model.accessors[skin.inverseBindMatrices];
  const auto& ibmView = model.bufferViews[ibmAcc.bufferView];
  const auto& ibmBuf = model.buffers[ibmView.buffer];
  const float* matPtr = reinterpret_cast<const float*>(
    &ibmBuf.data[ibmView.byteOffset + ibmAcc.byteOffset]);
  for (size_t i = 0; i < jcount; ++i) {
    outSkel.joints[i].name = model.nodes[skin.joints[i]].name;
    outSkel.joints[i].parentIndex = -1;
    // parent indices can be set by checking node hierarchy
    DirectX::XMFLOAT4X4 ibm;
    memcpy(&ibm, &matPtr[16 * i], sizeof(XMFLOAT4X4));
    outSkel.joints[i].bindPoseInverse = ibm;
  }
}

void GltfLoader::ParseAnimations(const tinygltf::Model& model, Skeleton& outSkel) {
  for (const auto& anim : model.animations) {
    SkeletonAnimation a;
    a.name = anim.name;
    float maxTime = 0;
    size_t jcount = outSkel.joints.size();
    a.channels.assign(jcount, {});
    for (const auto& channel : anim.channels) {
      int joint = channel.target_node;
      const auto& sampler = anim.samplers[channel.sampler];
      // input times
      const auto& tAcc = model.accessors[sampler.input];
      const auto& tView = model.bufferViews[tAcc.bufferView];
      const auto& tBuf = model.buffers[tView.buffer];
      const float* times = reinterpret_cast<const float*>(
        &tBuf.data[tView.byteOffset + tAcc.byteOffset]);
      // output transforms
      const auto& dAcc = model.accessors[sampler.output];
      const auto& dView = model.bufferViews[dAcc.bufferView];
      const auto& dBuf = model.buffers[dView.buffer];
      const float* mats = reinterpret_cast<const float*>(
        &dBuf.data[dView.byteOffset + dAcc.byteOffset]);
      for (size_t i = 0; i < tAcc.count; ++i) {
        SkeletonAnimationKey key;
        key.time = times[i];
        DirectX::XMFLOAT4X4 mat;
        memcpy(&mat, &mats[16 * i], sizeof(DirectX::XMFLOAT4X4));
        key.transform = mat;
        a.channels[joint].push_back(key);

        maxTime = std::max(maxTime, key.time);
        
      }
    }
    a.duration = maxTime;
    outSkel.animations.push_back(a);
  }
}

bool GltfLoader::Load(const std::string& filename, SkinMesh& outMesh, Skeleton& outSkel) {
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;
  std::string err, warn;
  bool isBinary = (filename.find(".glb") != std::string::npos);
  bool ret = isBinary ? loader.LoadBinaryFromFile(&model, &err, &warn, filename)
    : loader.LoadASCIIFromFile(&model, &err, &warn, filename);
  if (!warn.empty()) std::cerr << warn << std::endl;
  if (!err.empty()) std::cerr << err << std::endl;
  if (!ret) return false;
  ParseMesh(model, outMesh);
  ParseSkeleton(model, outSkel);
  ParseAnimations(model, outSkel);
  return true;
}