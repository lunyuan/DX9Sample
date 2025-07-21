// Step 0: AllocateHierarchy.cpp 必要 include // error check
#include <d3dx9.h>           // ID3DXAllocateHierarchy, D3DXFRAME, D3DXMESHCONTAINER
#include <DirectXMath.h>     // XMMatrixIdentity, XMStoreFloat4x4
#include <new>               // std::nothrow
#include <cstring>           // std::memcpy
#include <stdexcept>         // std::runtime_error
#include <algorithm>         // std::transform
#include <vector>            // std::vector
#include <cctype>            // ::tolower, ::toupper
#include "AllocateHierarchy.h"

// Step 1: Constructor 實作 // error check
AllocateHierarchy::AllocateHierarchy(IDirect3DDevice9* device) noexcept
  : m_device(device) {
}

// Step 2: CreateFrame 實作 // error check
STDMETHODIMP AllocateHierarchy::CreateFrame( LPCSTR Name, D3DXFRAME** ppNewFrame ) {
  if (!ppNewFrame) return E_POINTER;
  FrameEx* frame = new FrameEx();
  if (!frame) return E_OUTOFMEMORY;
  frame->Name = Name ? _strdup(Name) : nullptr;
  D3DXMatrixIdentity(&frame->TransformationMatrix);
  frame->CombinedTransform = frame->TransformationMatrix;
  DirectX::XMStoreFloat4x4(&frame->dxTransformationMatrix, DirectX::XMMatrixIdentity());
  DirectX::XMStoreFloat4x4(&frame->dxCombinedTransform, DirectX::XMMatrixIdentity());
  frame->pMeshContainer = nullptr;
  frame->pFrameSibling = nullptr;
  frame->pFrameFirstChild = nullptr;
  //// 將 FrameEx 指標轉換為 D3DXFRAME 指標
  //*ppNewFrame = reinterpret_cast<D3DXFRAME*>(frame);
  *ppNewFrame = frame;
  return S_OK;
}

// Step 3: CreateMeshContainer 實作 // error check
STDMETHODIMP AllocateHierarchy::CreateMeshContainer(
  LPCSTR Name,
  const D3DXMESHDATA* pMeshData,
  const D3DXMATERIAL* pMaterials,
  const D3DXEFFECTINSTANCE* pEffectInstances,
  DWORD numMaterials,
  const DWORD* pAdjacency,
  ID3DXSkinInfo* pSkinInfo,
  D3DXMESHCONTAINER** ppNewMeshContainer)  
{
  if (!pMeshData || !ppNewMeshContainer) return E_INVALIDARG;

  // 建立 MeshContainerEx
  MeshContainerEx* mc = new MeshContainerEx();
  if (!mc) return E_OUTOFMEMORY;

  mc->Name = Name ? _strdup(Name) : nullptr;
  mc->m_pSkinInfo = pSkinInfo;
  if (pSkinInfo) {
    mc->pSkinInfo = pSkinInfo; // 保存骨架資訊
    pSkinInfo->AddRef();
  }
  else {
    mc->m_pBoneOffsetMatrices = nullptr;
    mc->m_pBoneCombinationBuf = nullptr;
  }

  // CloneMeshFVF 需四參數
  HRESULT hr = pMeshData->pMesh->CloneMeshFVF(
    D3DXMESH_MANAGED | D3DXMESH_32BIT,
    pMeshData->pMesh->GetFVF(),
    m_device,
    &mc->MeshData.pMesh
  );
  if (FAILED(hr) || !mc->MeshData.pMesh) {
    delete mc;
    return hr;
  }
  
  // Also set m_pMesh for compatibility
  mc->m_pMesh = mc->MeshData.pMesh;
  // 儲存材質與貼圖名稱
  mc->NumMaterials = numMaterials;
  mc->m_Textures.resize(numMaterials);
  mc->m_pMaterials = new D3DMATERIAL9[numMaterials];
  for (UINT i = 0; i < numMaterials; i++) {
    if (pMaterials[i].pTextureFilename) {
      // 輸出調試訊息
      OutputDebugStringA(("Loading texture: " + std::string(pMaterials[i].pTextureFilename) + "\n").c_str());
      
      HRESULT texHr = D3DXCreateTextureFromFileA(
        m_device,
        pMaterials[i].pTextureFilename,
        &mc->m_Textures[i]);
        
      if (FAILED(texHr)) {
        OutputDebugStringA(("Failed to load texture: " + std::string(pMaterials[i].pTextureFilename) + " (HRESULT: 0x" + std::to_string(texHr) + ")\n").c_str());
        
        // 嘗試不同的大小寫組合
        std::string originalFilename(pMaterials[i].pTextureFilename);
        std::vector<std::string> caseVariations;
        
        // 1. 全部小寫
        std::string lowercase = originalFilename;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);
        caseVariations.push_back(lowercase);
        
        // 2. 首字母大寫，其餘小寫 (e.g., "Horse3.bmp")
        std::string titleCase = lowercase;
        if (!titleCase.empty()) {
          titleCase[0] = std::toupper(titleCase[0]);
        }
        caseVariations.push_back(titleCase);
        
        // 3. 全部大寫
        std::string uppercase = originalFilename;
        std::transform(uppercase.begin(), uppercase.end(), uppercase.begin(), ::toupper);
        caseVariations.push_back(uppercase);
        
        // 嘗試每個變體
        for (const auto& variant : caseVariations) {
          if (variant != originalFilename) {  // 避免重複嘗試原始名稱
            OutputDebugStringA(("Trying case variant: " + variant + "\n").c_str());
            texHr = D3DXCreateTextureFromFileA(
              m_device,
              variant.c_str(),
              &mc->m_Textures[i]);
              
            if (SUCCEEDED(texHr)) {
              OutputDebugStringA(("Texture loaded successfully with case variant: " + variant + "\n").c_str());
              break;
            }
          }
        }
        
        if (FAILED(texHr)) {
          // 如果所有變體都失敗，輸出最終錯誤
          OutputDebugStringA("Failed to load texture with all case variations\n");
        }
      } else {
        OutputDebugStringA("Texture loaded successfully\n");
      }
    }
    mc->m_pMaterials[i] = pMaterials[i].MatD3D;
  }
  // 備份 adjacency
  hr = D3DXCreateBuffer(numMaterials * 3 * sizeof(DWORD), &mc->m_pAdjacency);
  if (FAILED(hr) || !mc->m_pAdjacency) {
    delete[] mc->m_pMaterials;
    mc->MeshData.pMesh->Release();
    delete mc;
    return hr;
  }
  memcpy(mc->m_pAdjacency->GetBufferPointer(), pAdjacency, numMaterials * 3 * sizeof(DWORD));

  *ppNewMeshContainer = reinterpret_cast<D3DXMESHCONTAINER*>(mc);
  return S_OK;
}

// Step 4: DestroyFrame 實作 // error check
STDMETHODIMP AllocateHierarchy::DestroyFrame(D3DXFRAME* pFrame ) {
  if (!pFrame) return E_INVALIDARG;
  free(pFrame->Name);
  delete reinterpret_cast<FrameEx*>(pFrame);
  return S_OK;
}

// Step 5: DestroyMeshContainer 實作 // error check
STDMETHODIMP AllocateHierarchy::DestroyMeshContainer(
  D3DXMESHCONTAINER* pMeshContainer
) noexcept {
  if (!pMeshContainer) return E_INVALIDARG;
  auto* mc = reinterpret_cast<MeshContainerEx*>(pMeshContainer);
  if (mc->m_pSkinInfo)   mc->m_pSkinInfo->Release();
  for (auto tex : mc->m_Textures) if (tex) tex->Release();
  if (mc->m_pAdjacency) mc->m_pAdjacency->Release();
  if (mc->m_pBoneOffsetMatrices) mc->m_pBoneOffsetMatrices->Release();
  if (mc->m_pBoneCombinationBuf) mc->m_pBoneCombinationBuf->Release();
  if (mc->MeshData.pMesh) mc->MeshData.pMesh->Release();
  delete[] mc->m_pMaterials;
  delete mc;
  return S_OK;
}
