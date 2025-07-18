#pragma once

#include <d3d9.h>
#include <d3dx9mesh.h>
#include <DirectXMath.h>
#include <vector>
using namespace DirectX;

// 自訂頂點結構與 FVF 定義
struct Vertex
{
  XMFLOAT3 Position;
  XMFLOAT3 Normal;
  XMFLOAT2 TexCoord;
};
#define D3DFVF_VERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

// FrameEx 與 MeshContainerEx 定義（若需皮膚動畫）
struct FrameEx : public D3DXFRAME
{
  XMFLOAT4X4 CombinedTransform;
};

struct MeshContainerEx : public D3DXMESHCONTAINER
{
  ID3DXMesh* pOrigMesh;
  ID3DXSkinInfo* pSkinInfo;
  ID3DXMesh* pSkinMesh;
  std::vector<XMFLOAT4X4>  BoneOffset;
  std::vector<XMFLOAT4X4*> FrameMatrices;
  DWORD                    NumInfl;
};

class CD3DFileObject
{
public:
  CD3DFileObject();
  ~CD3DFileObject();

  // 載入靜態網格或骨架動畫網格
  HRESULT LoadFromX(
    IDirect3DDevice9* pd3dDevice,
    const WCHAR* szFileName,
    ID3DXAnimationController** ppAnimCtrl = nullptr
  );

  // 計算法線
  void ComputeNormals();

  // 更新骨架全局矩陣
  void UpdateFrameMatrices(
    FrameEx* pFrame,
    const XMMATRIX& parentMat
  );

  // 繪製靜態網格
  void RenderStatic(IDirect3DDevice9* pd3dDevice);

  // 繪製皮膚網格
  void RenderSkinned(
    IDirect3DDevice9* pd3dDevice,
    FrameEx* pFrameRoot,
    MeshContainerEx* pMeshContainer
  );

private:
  // for static mesh
  Vertex* m_pVertices;
  WORD* m_pIndices;
  DWORD               m_dwNumVertices;
  DWORD               m_dwNumFaces;
  XMFLOAT4X4          m_mat;
  D3DMATERIAL9* m_pMaterials;
  IDirect3DTexture9** m_pTextures;
  DWORD               m_dwNumMaterials;

  // for skinned mesh
  ID3DXAnimationController* m_pAnimCtrl;
  FrameEx* m_pFrameRoot;
  MeshContainerEx* m_pMeshContainer;
};
