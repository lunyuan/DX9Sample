#include "Loader.h"
#include "GltfLoader.h"
#include "XFileLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <d3d9.h>  // 為了 IDirect3DDevice9
#include <string>
#include <windows.h>

// 將 UTF-16 std::wstring 轉成 UTF-8 std::string
std::string WStringToString(const std::wstring& wstr)
{
  if (wstr.empty()) return std::string();

  // 第一次呼叫：取得所需 buffer 長度
  int size_needed = WideCharToMultiByte(
    CP_UTF8,            // 目標編碼 (UTF-8)
    0,
    wstr.data(),
    (int)wstr.size(),
    nullptr,
    0,
    nullptr,
    nullptr
  );
  std::string str(size_needed, 0);
  // 第二次呼叫：真正轉換
  WideCharToMultiByte(
    CP_UTF8,
    0,
    wstr.data(),
    (int)wstr.size(),
    str.data(),
    size_needed,
    nullptr,
    nullptr
  );
  return str;
}


bool Loader::LoadMesh(const std::wstring& filename, SkinMesh& outMesh) {
  // 讀取自訂 .mesh
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    std::wcerr << L"Loader::LoadMesh 無法開啟檔案: " << filename << std::endl;
    return false;
  }
  uint32_t vcount = 0, icount = 0;
  ifs.read(reinterpret_cast<char*>(&vcount), sizeof(vcount));
  ifs.read(reinterpret_cast<char*>(&icount), sizeof(icount));
  outMesh.vertices.resize(vcount);
  outMesh.indices.resize(icount);
  ifs.read(reinterpret_cast<char*>(outMesh.vertices.data()), vcount * sizeof(outMesh.vertices[0]));
  ifs.read(reinterpret_cast<char*>(outMesh.indices.data()), icount * sizeof(outMesh.indices[0]));
  return true;
}
bool Loader::LoadSkeleton(const std::wstring& filename, Skeleton& outSkel) {
  // 讀取自訂 .skel
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    std::wcerr << "Loader::LoadSkeleton 無法開啟檔案: " << filename << std::endl;
    return false;
  }
  uint32_t jcount = 0;
  ifs.read(reinterpret_cast<char*>(&jcount), sizeof(jcount));
  outSkel.joints.resize(jcount);
  for (uint32_t i = 0; i < jcount; ++i) {
    auto& j = outSkel.joints[i];
    uint32_t nameLen = 0;
    ifs.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    j.name.resize(nameLen);
    ifs.read(&j.name[0], nameLen);
    ifs.read(reinterpret_cast<char*>(&j.parentIndex), sizeof(j.parentIndex));
    ifs.read(reinterpret_cast<char*>(&j.bindPoseInverse), sizeof(j.bindPoseInverse));
  }
  return true;
}
bool Loader::LoadAnimation(const std::wstring& filename, Skeleton& outSkel) {
  // 讀取自訂 .anim
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    std::wcerr << L"Loader::LoadAnimation 無法開啟檔案: " << filename << std::endl;
    return false;
  }
  uint32_t animCount = 0;
  ifs.read(reinterpret_cast<char*>(&animCount), sizeof(animCount));
  outSkel.animations.clear();
  outSkel.animations.resize(animCount);
  for (uint32_t i = 0; i < animCount; ++i) {
    auto& anim = outSkel.animations[i];
    uint32_t nameLen = 0;
    ifs.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    anim.name.resize(nameLen);
    ifs.read(&anim.name[0], nameLen);
    ifs.read(reinterpret_cast<char*>(&anim.duration), sizeof(anim.duration));
    uint32_t jointCount = 0;
    ifs.read(reinterpret_cast<char*>(&jointCount), sizeof(jointCount));
    anim.channels.resize(jointCount);
    for (uint32_t j = 0; j < jointCount; ++j) {
      auto& channel = anim.channels[j];
      uint32_t keyCount = 0;
      ifs.read(reinterpret_cast<char*>(&keyCount), sizeof(keyCount));
      channel.resize(keyCount);
      for (uint32_t k = 0; k < keyCount; ++k) {
        auto& key = channel[k];
        ifs.read(reinterpret_cast<char*>(&key.time), sizeof(key.time));
        ifs.read(reinterpret_cast<char*>(&key.transform), sizeof(key.transform));
      }
    }
  }
  return true;
}
bool Loader::LoadGltf(const std::string& filename, SkinMesh& outMesh, Skeleton& outSkel) {
  return GltfLoader::Load(filename, outMesh, outSkel);
}

/////////////////////////
// 委派到 .x 檔載入
/////////////////////////
bool Loader::LoadXFile(const std::wstring& filename, IDirect3DDevice9* dev, SkinMesh& outMesh, Skeleton& outSkel) {
  return XFileLoader::Load(filename, dev, outMesh, outSkel);
}

/////////////////////////
// 根據副檔名自動分發
/////////////////////////
bool Loader::Load(const std::wstring& filename, IDirect3DDevice9* dev, SkinMesh& outMesh, Skeleton& outSkel) {
  auto pos = filename.find_last_of('.');
  if (pos == std::wstring::npos) {
    std::wcerr << "Loader::Load 缺少副檔名: " << filename << std::endl;
    return false;
  }
  std::wstring ext = filename.substr(pos + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (ext == L"mesh") {
    return LoadMesh(filename, outMesh);
  }
  else if (ext == L"skel") {
    return LoadSkeleton(filename, outSkel);
  }
  else if (ext == L"anim") {
    return LoadAnimation(filename, outSkel);
  }
  else if (ext == L"gltf" || ext == L"glb") {
    return LoadGltf(WStringToString(filename), outMesh, outSkel);
  }
  else if (ext == L"x") {
    return LoadXFile(filename, dev, outMesh, outSkel);
  }
  else {
    std::wcerr << "Loader::Load 不支援副檔名: " << ext << std::endl;
    return false;
  }
}