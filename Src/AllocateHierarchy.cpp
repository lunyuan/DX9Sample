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
#include <memory>            // std::unique_ptr, std::make_unique
#include "AllocateHierarchy.h"

// Step 1: Constructor 實作 // error check
AllocateHierarchy::AllocateHierarchy(IDirect3DDevice9* device) noexcept
  : m_device(device) {
}

// Step 2: CreateFrame 實作 // error check
STDMETHODIMP AllocateHierarchy::CreateFrame( LPCSTR Name, D3DXFRAME** ppNewFrame ) {
  if (!ppNewFrame) return E_POINTER;
  // Use unique_ptr for exception safety, release at the end
  auto frame = std::make_unique<FrameEx>();
  
  // Use std::string instead of _strdup
  if (Name) {
    size_t len = strlen(Name) + 1;
    frame->Name = new char[len];
    strcpy_s(frame->Name, len, Name);
  } else {
    frame->Name = nullptr;
  }
  D3DXMatrixIdentity(&frame->TransformationMatrix);
  frame->CombinedTransform = frame->TransformationMatrix;
  DirectX::XMStoreFloat4x4(&frame->dxTransformationMatrix, DirectX::XMMatrixIdentity());
  DirectX::XMStoreFloat4x4(&frame->dxCombinedTransform, DirectX::XMMatrixIdentity());
  frame->pMeshContainer = nullptr;
  frame->pFrameSibling = nullptr;
  frame->pFrameFirstChild = nullptr;
  //// 將 FrameEx 指標轉換為 D3DXFRAME 指標
  //*ppNewFrame = reinterpret_cast<D3DXFRAME*>(frame);
  *ppNewFrame = frame.release(); // Transfer ownership to DirectX
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

  // 建立 MeshContainerEx with unique_ptr for exception safety
  auto mc = std::make_unique<MeshContainerEx>();

  // Use proper string allocation instead of _strdup
  if (Name) {
    size_t len = strlen(Name) + 1;
    mc->Name = new char[len];
    strcpy_s(mc->Name, len, Name);
  } else {
    mc->Name = nullptr;
  }
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
    return hr; // unique_ptr will clean up automatically
  }
  
  // Also set m_pMesh for compatibility
  mc->m_pMesh = mc->MeshData.pMesh;
  // 儲存材質與貼圖名稱
  mc->NumMaterials = numMaterials;
  mc->m_Textures.resize(numMaterials);
  mc->m_TextureFileNames.resize(numMaterials);
  // Use std::vector instead of raw arrays
  auto materials = std::make_unique<D3DMATERIAL9[]>(numMaterials);
  auto dxMaterials = std::make_unique<D3DXMATERIAL[]>(numMaterials);
  mc->m_pMaterials = materials.get();
  mc->pMaterials = dxMaterials.get();
  
  for (UINT i = 0; i < numMaterials; i++) {
    // 複製材質資料
    mc->pMaterials[i].MatD3D = pMaterials[i].MatD3D;
    mc->pMaterials[i].pTextureFilename = nullptr;  // DirectX 不需要，但我們保存到 m_TextureFileNames
    mc->m_pMaterials[i] = pMaterials[i].MatD3D;
    // 載入材質中的貼圖
    if (pMaterials[i].pTextureFilename != nullptr) {
      // 保存檔名
      mc->m_TextureFileNames[i] = pMaterials[i].pTextureFilename;
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
    mc->MeshData.pMesh->Release();
    return hr; // unique_ptr will clean up automatically
  }
  memcpy(mc->m_pAdjacency->GetBufferPointer(), pAdjacency, numMaterials * 3 * sizeof(DWORD));

  // Release the arrays to DirectX ownership
  materials.release();
  dxMaterials.release();
  
  *ppNewMeshContainer = reinterpret_cast<D3DXMESHCONTAINER*>(mc.release());
  return S_OK;
}

// Step 4: DestroyFrame 實作 // error check
STDMETHODIMP AllocateHierarchy::DestroyFrame(D3DXFRAME* pFrame ) {
  if (!pFrame) return E_INVALIDARG;
  delete[] pFrame->Name; // Changed from free to delete[] to match allocation
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
