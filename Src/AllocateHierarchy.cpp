// Step 0: AllocateHierarchy.cpp 必要 include // error check
#include <d3dx9.h>           // ID3DXAllocateHierarchy, D3DXFRAME, D3DXMESHCONTAINER
#include <DirectXMath.h>     // XMMatrixIdentity, XMStoreFloat4x4
#include <new>               // std::nothrow
#include <cstring>           // std::memcpy
#include <stdexcept>         // std::runtime_error
#include <algorithm>         // std::transform
#include <iostream>
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
  
  // 設定基類的 pMaterials 指標
  mc->pMaterials = new D3DXMATERIAL[numMaterials];
  
  for (UINT i = 0; i < numMaterials; i++) {
    // 複製材質資料
    mc->pMaterials[i].MatD3D = pMaterials[i].MatD3D;
    mc->pMaterials[i].pTextureFilename = nullptr;  // 我們不需要檔名，已經載入貼圖
    mc->m_pMaterials[i] = pMaterials[i].MatD3D;
    // 載入材質中的貼圖
    if (pMaterials[i].pTextureFilename != nullptr) {
      char debugMsg[512];
      sprintf_s(debugMsg, "AllocateHierarchy: Attempting to load texture: %s\n", pMaterials[i].pTextureFilename);
      OutputDebugStringA(debugMsg);
      
      // 嘗試原始檔名
      HRESULT hr = D3DXCreateTextureFromFileA(
        m_device,
        pMaterials[i].pTextureFilename,
        &mc->m_Textures[i]
      );
      
      // 如果失敗，嘗試小寫版本
      if (FAILED(hr)) {
        std::string lowerName = pMaterials[i].pTextureFilename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        sprintf_s(debugMsg, "AllocateHierarchy: Original failed, trying lowercase: %s\n", lowerName.c_str());
        OutputDebugStringA(debugMsg);
        
        hr = D3DXCreateTextureFromFileA(
          m_device,
          lowerName.c_str(),
          &mc->m_Textures[i]
        );
      }
      
      // 如果還是失敗，使用替代貼圖
      if (FAILED(hr)) {
        // 檢查是否為 RED.BMP，使用 Horse4.bmp 作為替代
        std::string filename = pMaterials[i].pTextureFilename;
        std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
        
        if (filename == "RED.BMP") {
          sprintf_s(debugMsg, "AllocateHierarchy: RED.BMP not found, using Horse4.bmp as fallback\n");
          OutputDebugStringA(debugMsg);
          
          hr = D3DXCreateTextureFromFileA(
            m_device,
            "Horse4.bmp",
            &mc->m_Textures[i]
          );
        }
      }
      
      if (FAILED(hr)) {
        mc->m_Textures[i] = nullptr;
        sprintf_s(debugMsg, "AllocateHierarchy: Failed to load texture: %s (HRESULT: 0x%08X)\n", 
                  pMaterials[i].pTextureFilename, hr);
        OutputDebugStringA(debugMsg);
      } else {
        sprintf_s(debugMsg, "AllocateHierarchy: Successfully loaded texture: %s (ptr: %p)\n", 
                  pMaterials[i].pTextureFilename, mc->m_Textures[i]);
        OutputDebugStringA(debugMsg);
      }
    } else {
      mc->m_Textures[i] = nullptr;
    }
    
    // 如果需要調試，可以輸出被跳過的貼圖名稱
    if (pMaterials[i].pTextureFilename) {
      // std::cout << "Skipping texture from model: " << pMaterials[i].pTextureFilename << std::endl;
    }
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
  delete[] mc->pMaterials;  // 釋放基類的材質陣列
  delete mc;
  return S_OK;
}
