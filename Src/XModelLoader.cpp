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

std::vector<std::string> XModelLoader::GetModelNames(
  const std::filesystem::path& file) const {
  
  // 創建一個臨時設備來進行檢查 (這個實作比較簡化)
  // 實際應用中可能需要更輕量級的方法來解析 .x 檔案結構
  
  // 為了簡化實作，我們先載入檔案然後提取名稱
  // 注意：這需要一個有效的 D3D 設備，在實際應用中應該有更好的方法
  
  std::vector<std::string> modelNames;
  
  try {
    // 建立一個簡單的 D3D 實例來解析檔案結構
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return modelNames;
    
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.hDeviceWindow = GetDesktopWindow();
    
    IDirect3DDevice9* tempDevice = nullptr;
    HRESULT hr = d3d->CreateDevice(
      D3DADAPTER_DEFAULT,
      D3DDEVTYPE_REF, // 使用參考設備，速度慢但相容性好
      GetDesktopWindow(),
      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
      &pp,
      &tempDevice
    );
    
    if (SUCCEEDED(hr) && tempDevice) {
      AllocateHierarchy alloc(tempDevice);
      ID3DXAnimationController* animCtrl = nullptr;
      FrameEx* root = nullptr;
      
      hr = D3DXLoadMeshHierarchyFromX(
        file.wstring().c_str(),
        D3DXMESH_MANAGED,
        tempDevice,
        &alloc,
        nullptr,
        reinterpret_cast<D3DXFRAME**>(&root),
        &animCtrl
      );
      
      if (SUCCEEDED(hr)) {
        DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
        UpdateCombined(root, identity);
        std::vector<FrameEx*> frames;
        std::vector<int> parents;
        CollectFrames(root, frames, parents, -1);
        
        // 收集所有包含 mesh 的 frame 名稱
        for (auto* f : frames) {
          if (f->pMeshContainer) {
            modelNames.push_back(f->Name);
          }
        }
        
        alloc.DestroyFrame(reinterpret_cast<D3DXFRAME*>(root));
      }
      
      if (animCtrl) animCtrl->Release();
      tempDevice->Release();
    }
    
    d3d->Release();
  }
  catch (...) {
    // 如果解析失敗，返回空列表
  }
  
  return modelNames;
}
