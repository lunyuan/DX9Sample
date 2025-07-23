#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <new>                 // std::nothrow
#include <cstring>             // std::memcpy
#include <iostream>            // std::cerr


struct MeshContainerEx : public D3DXMESHCONTAINER {
  // COM interfaces - don't use smart pointers for these
  IDirect3DDevice9* m_pDevice = nullptr;
  ID3DXMesh* m_pMesh = nullptr;
  ID3DXSkinInfo* m_pSkinInfo = nullptr;       // 骨架動畫資訊
  
  // Raw pointer for array allocated by DirectX (ownership transferred)
  D3DMATERIAL9* m_pMaterials = nullptr;
  
  // Use std::vector for automatic memory management
  std::vector<IDirect3DTexture9*> m_Textures{};     // 材質貼圖 (COM objects)
  std::vector<std::string> m_TextureFileNames{};     // 貼圖檔名
  
  // COM interfaces
  ID3DXBuffer* m_pAdjacency = nullptr;      // adjacency info
  ID3DXBuffer* m_pBoneOffsetMatrices = nullptr; // 骨骼偏移矩陣
  ID3DXBuffer* m_pBoneCombinationBuf = nullptr; // 組合矩陣
  
  MeshContainerEx() = default;
};
struct FrameEx : public D3DXFRAME {
  D3DXMATRIX CombinedTransform;
  DirectX::XMFLOAT4X4 dxTransformationMatrix;
  DirectX::XMFLOAT4X4 dxCombinedTransform;
  FrameEx() {
    DirectX::XMStoreFloat4x4(&dxTransformationMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&dxCombinedTransform, DirectX::XMMatrixIdentity());
    pMeshContainer = nullptr;
    pFrameSibling = nullptr;
    pFrameFirstChild = nullptr;
  }
};


