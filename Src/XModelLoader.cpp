#include "XModelLoader.h"
#include "AllocateHierarchy.h"
#include "Utilities.h"
#include "SkinMeshFactory.h"
#include <DirectXMath.h>
#include <d3dx9.h>
#include <format>
#include <stdexcept>

std::map<std::string, ModelData> XModelLoader::Load(
  const std::filesystem::path& file,
  IDirect3DDevice9* device) const {
  if (!device) throw std::invalid_argument("device is null");
  AllocateHierarchy alloc(device);
  ID3DXAnimationController* animCtrl = nullptr;
  FrameEx* root = nullptr;
  HRESULT hr = D3DXLoadMeshHierarchyFromX(
    file.wstring().c_str(),
    D3DXMESH_MANAGED,
    device,
    &alloc,
    nullptr,
    reinterpret_cast<D3DXFRAME**>(&root),
    &animCtrl
  );
  if (FAILED(hr)) throw std::runtime_error(std::format("Load X failed: 0x{:X}", hr));

  DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
  UpdateCombined(root, identity);
  std::vector<FrameEx*> frames;
  std::vector<int> parents;
  CollectFrames(root, frames, parents, -1);

  Skeleton skel;
  skel.joints.reserve(frames.size());
  for (size_t i = 0; i < frames.size(); ++i) {
    skel.joints.push_back({ frames[i]->Name, parents[i] });
  }

  std::map<std::string, ModelData> result;
  for (auto* f : frames) {
    if (!f->pMeshContainer) continue;
    auto* mc = reinterpret_cast<MeshContainerEx*>(f->pMeshContainer);
    ModelData md;
    md.mesh = CreateSkinMesh(device, mc);
    md.skeleton = skel;
    md.animController = std::shared_ptr<ID3DXAnimationController>(animCtrl, [](auto*) {});
    result[f->Name] = md;
  }

  alloc.DestroyFrame(reinterpret_cast<D3DXFRAME*>(root));
  return result;
}
