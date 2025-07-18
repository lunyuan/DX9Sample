#include "d3dfile.h"

#pragma comment(lib, "d3dx9.lib")

CD3DFileObject::CD3DFileObject()
  : m_pVertices(nullptr)
  , m_pIndices(nullptr)
  , m_dwNumVertices(0)
  , m_dwNumFaces(0)
  , m_pMaterials(nullptr)
  , m_pTextures(nullptr)
  , m_dwNumMaterials(0)
  , m_pAnimCtrl(nullptr)
  , m_pFrameRoot(nullptr)
  , m_pMeshContainer(nullptr)
{
  XMStoreFloat4x4(&m_mat, XMMatrixIdentity());
}

CD3DFileObject::~CD3DFileObject()
{
  delete[] m_pVertices;
  delete[] m_pIndices;
  delete[] m_pMaterials;
  if (m_pTextures) {
    for (DWORD i = 0; i < m_dwNumMaterials; ++i)
      if (m_pTextures[i]) m_pTextures[i]->Release();
    delete[] m_pTextures;
  }
  if (m_pAnimCtrl) m_pAnimCtrl->Release();
  // 釋放 FrameEx 與 MeshContainerEx
  D3DXFrameDestroy(reinterpret_cast<D3DXFRAME*>(m_pFrameRoot), nullptr);
}

HRESULT CD3DFileObject::LoadFromX(
  IDirect3DDevice9* pd3dDevice,
  const WCHAR* szFileName,
  ID3DXAnimationController** ppAnimCtrl
)
{
  // 載入網格與動畫控制器
  HRESULT hr = D3DXLoadMeshHierarchyFromX(
    szFileName,
    D3DXMESH_MANAGED,
    pd3dDevice,
    nullptr,       // Allocator
    nullptr,       // EffectInstances
    reinterpret_cast<D3DXFRAME**>(&m_pFrameRoot),
    &m_pAnimCtrl
  );
  if (FAILED(hr)) return hr;
  if (ppAnimCtrl) *ppAnimCtrl = m_pAnimCtrl;

  // 找第一個 MeshContainerEx
  m_pMeshContainer = reinterpret_cast<MeshContainerEx*>(m_pFrameRoot->pMeshContainer);
  return S_OK;
}

void CD3DFileObject::ComputeNormals()
{
  // 適用於靜態網格，若無靜態直接返回
  std::vector<XMFLOAT3> accum(m_dwNumVertices, XMFLOAT3(0, 0, 0));

  for (DWORD f = 0; f < m_dwNumFaces; ++f) {
    WORD i0 = m_pIndices[f * 3 + 0];
    WORD i1 = m_pIndices[f * 3 + 1];
    WORD i2 = m_pIndices[f * 3 + 2];

    XMVECTOR v0 = XMLoadFloat3(&m_pVertices[i0].Position);
    XMVECTOR v1 = XMLoadFloat3(&m_pVertices[i1].Position);
    XMVECTOR v2 = XMLoadFloat3(&m_pVertices[i2].Position);
    XMVECTOR n = XMVector3Normalize(XMVector3Cross(v1 - v0, v2 - v0));

    // 累加
    XMVECTOR a0 = XMLoadFloat3(&accum[i0]) + n;
    XMVECTOR a1 = XMLoadFloat3(&accum[i1]) + n;
    XMVECTOR a2 = XMLoadFloat3(&accum[i2]) + n;
    XMStoreFloat3(&accum[i0], a0);
    XMStoreFloat3(&accum[i1], a1);
    XMStoreFloat3(&accum[i2], a2);
  }

  // 正規化並寫回頂點
  for (DWORD i = 0; i < m_dwNumVertices; ++i) {
    XMVECTOR norm = XMVector3Normalize(XMLoadFloat3(&accum[i]));
    XMStoreFloat3(&m_pVertices[i].Normal, norm);
  }
}

void CD3DFileObject::UpdateFrameMatrices(
  FrameEx* pFrame,
  const XMMATRIX& parentMat
)
{
  XMMATRIX local = XMLoadFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&pFrame->TransformationMatrix));
  XMMATRIX world = local * parentMat;
  XMStoreFloat4x4(&pFrame->CombinedTransform, world);
  if (pFrame->pFrameFirstChild)   UpdateFrameMatrices((FrameEx*)pFrame->pFrameFirstChild, world);
  if (pFrame->pFrameSibling)      UpdateFrameMatrices((FrameEx*)pFrame->pFrameSibling, parentMat);
}

void CD3DFileObject::RenderStatic(IDirect3DDevice9* pd3dDevice)
{
  pd3dDevice->SetFVF(D3DFVF_VERTEX);

  // 設定世界矩陣
  D3DMATRIX mWorld;
  XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&mWorld), XMLoadFloat4x4(&m_mat));
  pd3dDevice->SetTransform(D3DTS_WORLD, &mWorld);

  // 逐材質繪製
  for (DWORD i = 0; i < m_dwNumMaterials; ++i) {
    pd3dDevice->SetMaterial(&m_pMaterials[i]);
    pd3dDevice->SetTexture(0, m_pTextures[i]);
    pd3dDevice->DrawIndexedPrimitive(
      D3DPT_TRIANGLELIST,
      0,
      0,
      m_dwNumVertices,
      i * (m_dwNumFaces / m_dwNumMaterials) * 3,
      m_dwNumFaces / m_dwNumMaterials
    );
  }
}

void CD3DFileObject::RenderSkinned(
  IDirect3DDevice9* pd3dDevice,
  FrameEx* pFrameRoot,
  MeshContainerEx* pMeshContainer
)
{
  // 更新動畫
  m_pAnimCtrl->AdvanceTime(0.016f, nullptr);
  UpdateFrameMatrices(pFrameRoot, XMMatrixIdentity());

  // 計算並上傳骨骼矩陣
  UINT numBones = pMeshContainer->NumInfl;
  std::vector<XMFLOAT4X4> blendMatrices(numBones);
  for (UINT i = 0; i < numBones; ++i) {
    XMMATRIX offset = XMLoadFloat4x4(&pMeshContainer->BoneOffset[i]);
    XMMATRIX bone = XMLoadFloat4x4(pMeshContainer->FrameMatrices[i]);
    XMMATRIX final = offset * bone;
    XMStoreFloat4x4(&blendMatrices[i], final);
  }
  pd3dDevice->SetVertexShaderConstantF(0, reinterpret_cast<float*>(&blendMatrices[0]), numBones * 3);

  // 繪製
  pMeshContainer->pSkinMesh->DrawSubset(0);
}