#include "XFileLoader.h"
#include <d3dx9.h>             // D3DXLoadMeshHierarchyFromX, ID3DXAllocateHierarchy
#include <DirectXMath.h>       // DirectX::XMMatrixIdentity
#include <new>                 // std::nothrow
#include <cstring>             // std::memcpy
#include <iostream>            // std::cerr
#include "AllocateHierarchy.h"
using namespace DirectX;

extern IDirect3DDevice9* g_pd3dDevice; // 全域 Direct3D 裝置
//// 擴充 D3DXFRAME 以儲存合併後的全域矩陣
//struct FrameEx : public D3DXFRAME {
//  D3DXMATRIX CombinedTransform;
//  FrameEx() {
//    pMeshContainer = nullptr;
//    pFrameSibling = nullptr;
//    pFrameFirstChild = nullptr;
//    D3DXMatrixIdentity(&TransformationMatrix);
//   // CombinedTransform = XMMatrixIdentity();
//  }
//};
//
//struct MeshContainerEx : public D3DXMESHCONTAINER {
//    D3DMATERIAL9* m_pMaterials = nullptr;
//    IDirect3DDevice9 * m_pDevice = nullptr;
//    ID3DXSkinInfo* m_pSkinInfo = nullptr;       // 骨架動畫資訊
//    std::vector<IDirect3DTexture9*> m_Textures{};     // 材質貼圖
//    ID3DXBuffer* m_pAdjacency = nullptr;      // adjacency info
//    ID3DXBuffer* m_pBoneOffsetMatrices = nullptr; // 骨骼偏移矩陣
//    ID3DXBuffer* m_pBoneCombinationBuf = nullptr; // 組合矩陣
//};
//
//// 自訂 AllocationHierarchy，讓 D3DXLoadMeshHierarchyFromX 幫我們 build FrameEx
//class AllocateHierarchy : public ID3DXAllocateHierarchy {
//public:
//  STDMETHOD(CreateFrame)(LPCSTR Name, D3DXFRAME** ppNewFrame) {
//    if (!ppNewFrame) return E_POINTER;
//    FrameEx* f = new FrameEx();
//    if (!f) return E_OUTOFMEMORY;
//    f->Name = Name ? _strdup(Name) : nullptr;
//    D3DXMatrixIdentity(&f->TransformationMatrix);
//    f->CombinedTransform = f->TransformationMatrix;
//    f->pMeshContainer = nullptr;
//    f->pFrameSibling = nullptr;
//    f->pFrameFirstChild = nullptr;
//    *ppNewFrame = f;
//    return S_OK;
//  }
//  STDMETHOD(CreateMeshContainer)(
//    LPCSTR Name,
//    const D3DXMESHDATA* meshData,
//    const D3DXMATERIAL* materials,
//    const D3DXEFFECTINSTANCE* fx,
//    DWORD numMaterials,
//    const DWORD* adjacency,
//    ID3DXSkinInfo* skinInfo,
//    D3DXMESHCONTAINER** ppNewMeshContainer)
//  {
//    MeshContainerEx* mc = new MeshContainerEx();
//    memset(mc, 0, sizeof(MeshContainerEx));
//    mc->Name = Name ? _strdup(Name) : nullptr;
//    if(skinInfo)  {
//      mc->pSkinInfo = skinInfo; // 保存骨架資訊
//      skinInfo->AddRef();
//    } else {
//      mc->m_pBoneOffsetMatrices = nullptr;
//      mc->m_pBoneCombinationBuf = nullptr;
//    }
//    // 複製 Mesh
//    meshData->pMesh->CloneMeshFVF(
//      meshData->pMesh->GetOptions(),
//      meshData->pMesh->GetFVF(),
//      g_pd3dDevice,
//      &mc->MeshData.pMesh);
//    // 儲存材質與貼圖名稱
//    mc->NumMaterials = numMaterials;
//    mc->m_Textures.resize(numMaterials);
//    mc->m_pMaterials = new D3DMATERIAL9[numMaterials];
//    for (UINT i = 0; i < numMaterials; i++) {
//      if (materials[i].pTextureFilename) {
//        D3DXCreateTextureFromFileA(
//          g_pd3dDevice,
//          materials[i].pTextureFilename,
//          &mc->m_Textures[i]);
//      }
//      mc->m_pMaterials[i] = materials[i].MatD3D;
//    }
//    // 備份 adjacency
//    D3DXCreateBuffer(numMaterials * 3 * sizeof(DWORD), &mc->m_pAdjacency);
//    memcpy(mc->m_pAdjacency->GetBufferPointer(), adjacency, numMaterials * 3 * sizeof(DWORD));
//
//    //// 複製 Mesh
//    //meshData->pMesh->CloneMeshFVF(
//    //  meshData->pMesh->GetOptions(),
//    //  meshData->pMesh->GetFVF(),
//    //  m_pDevice,
//    //  &mc->MeshData.pMesh);
//    //
//    // 我們只需要 Frame hierarchy 來做骨架，MeshContainer 使用你原本的方式或留空
//    *ppNewMeshContainer = mc;
//    return S_OK;
//  }
//  STDMETHOD(DestroyFrame)(D3DXFRAME* pFrameToFree) {
//    if (!pFrameToFree) return S_OK;
//    free(pFrameToFree->Name);
//    delete pFrameToFree;
//    return S_OK;
//  }
//  STDMETHOD(DestroyMeshContainer)(D3DXMESHCONTAINER* pContainer) {
//    MeshContainerEx* p = static_cast<MeshContainerEx*>(pContainer);
//    if (p->pSkinInfo) p->pSkinInfo->Release(); 
//    for (auto tex : p->m_Textures) if (tex) tex->Release();
//    if (p->m_pAdjacency) p->m_pAdjacency->Release();
//    if (p->m_pBoneOffsetMatrices) p->m_pBoneOffsetMatrices->Release();
//    if (p->m_pBoneCombinationBuf) p->m_pBoneCombinationBuf->Release();
//    if(p->MeshData.pMesh) p->MeshData.pMesh->Release();
//    delete[] p->pMaterials;
//    delete p;
//    return S_OK;
//  }
//};

// 遞迴搜尋 Frame
static FrameEx* FindFrameByName(FrameEx* pFrame, const char* targetName) {
  if (pFrame->Name && strcmp(pFrame->Name, targetName) == 0) {
    return pFrame;
  }
  if (pFrame->pFrameSibling) {
    FrameEx* found = FindFrameByName(static_cast<FrameEx*>(pFrame->pFrameSibling), targetName);
    if (found) return found;
  }
  if (pFrame->pFrameFirstChild) {
    FrameEx* found = FindFrameByName(static_cast<FrameEx*>(pFrame->pFrameFirstChild), targetName);
    if (found) return found;
  }
  return nullptr;
}

// 更新每個 FrameEx 的 CombinedTransform
static void UpdateCombined(FrameEx* f, const D3DXMATRIX& parentMat) {
  // 合併：本地 * 父級
  D3DXMatrixMultiply(&f->CombinedTransform,
    &f->TransformationMatrix,
    &parentMat);
  if (f->pFrameSibling)
    UpdateCombined(static_cast<FrameEx*>(f->pFrameSibling), parentMat);
  if (f->pFrameFirstChild)
    UpdateCombined(static_cast<FrameEx*>(f->pFrameFirstChild), f->CombinedTransform);
}

// 遞迴收集所有 FrameEx 並記錄父索引
static void CollectFrames(FrameEx* f,
  std::vector<FrameEx*>& outFrames,
  std::vector<int>& outParents,
  int                     parentIndex)
{
  int myIndex = static_cast<int>(outFrames.size());
  outFrames.push_back(f);
  outParents.push_back(parentIndex);

  if (f->pFrameSibling)
    CollectFrames(static_cast<FrameEx*>(f->pFrameSibling),
      outFrames, outParents, parentIndex);
  if (f->pFrameFirstChild)
    CollectFrames(static_cast<FrameEx*>(f->pFrameFirstChild),
      outFrames, outParents, myIndex);
}



bool XFileLoader::Load(const std::wstring& file, IDirect3DDevice9* dev, SkinMesh& mesh, Skeleton& outSkel) {
  ID3DXMesh* xmesh = nullptr;
  ID3DXBuffer* adjacency = nullptr;
  ID3DXBuffer* materialBuffer = nullptr;
  DWORD numMaterials = 0;
  HRESULT hr = D3DXLoadMeshFromX(
    file.c_str(),
    D3DXMESH_MANAGED,
    dev,
    &adjacency,
    &materialBuffer,
    nullptr,
    &numMaterials,
    &xmesh
  );
  if (FAILED(hr)) {
    char buf[256] = {};
    FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, hr,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buf, sizeof(buf), nullptr
    );
    std::cerr
      << "D3DXLoadMeshFromX 失敗，HRESULT=0x"
      << std::hex << hr
      << " (" << buf << ")"
      << std::endl;
    return false;
  }
  ID3DXMesh* mesh32 = nullptr;
  hr = xmesh->CloneMeshFVF(
    xmesh->GetOptions() | D3DXMESH_32BIT,  // 指定 32-bit 索引
    xmesh->GetFVF(),
    dev,
    &mesh32
  );
  xmesh->Release();
  xmesh = mesh32;


  mesh.LoadMaterials(dev, materialBuffer, numMaterials);

  //// 2. 取得並解析材質
  //auto mats = reinterpret_cast<D3DXMATERIAL*>(materialBuffer->GetBufferPointer());
  //for (DWORD i = 0; i < numMaterials; ++i) {
  //  // mats[i].MatD3D: D3DMATERIAL9
  //  // mats[i].pTextureFilename: char*
  //  std::string texFile = mats[i].pTextureFilename;
  //  // 載入貼圖...
  //}

  // Retrieve and copy vertex data

  UINT numVerts = xmesh->GetNumVertices();

  IDirect3DVertexBuffer9* vb = nullptr;
  xmesh->GetVertexBuffer(&vb);
  void* vdata = nullptr;
  vb->Lock(0, 0, &vdata, 0);
  mesh.vertices.resize(xmesh->GetNumVertices());
  memcpy(mesh.vertices.data(), vdata, mesh.vertices.size() * sizeof(Vertex));

  // 3. 假設下列 offset 與 xmesh 的 FVF 對應 (常見 D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)
  const UINT stride = xmesh->GetNumBytesPerVertex();
  const UINT posOffset = 0;                       // XYZ 從 0 開始
  const UINT normalOffset = sizeof(float) * 3;         // XYZ(12 bytes) 後面接 Normals
  const UINT uvOffset = normalOffset + sizeof(float) * 3; // 再接 Normals(12 bytes)
  

  // 4. 逐筆複製
  for (UINT i = 0; i < numVerts; ++i) {
    BYTE* src = reinterpret_cast<BYTE*>(vdata) + i * stride;
    // Position
    memcpy(&mesh.vertices[i].pos, src + posOffset, sizeof(mesh.vertices[i].pos));
    // Normal
    memcpy(&mesh.vertices[i].norm, src + normalOffset, sizeof(mesh.vertices[i].norm));
    // UV
    memcpy(&mesh.vertices[i].uv, src + uvOffset, sizeof(mesh.vertices[i].uv));

    // 其他欄位用預設
    //mesh.vertices[i].rhw = 1.0f;
    mesh.vertices[i].col = 0xFFFFFFFF;
    mesh.vertices[i].spec = 0;
    mesh.vertices[i].weights = { 1,0,0,0 };
    memset(&mesh.vertices[i].boneIndices, 0, 4);
  }

  vb->Unlock();
  vb->Release();



  // Retrieve and copy index data
  IDirect3DIndexBuffer9* ib = nullptr;
  xmesh->GetIndexBuffer(&ib);
  D3DINDEXBUFFER_DESC ibDesc;
  ib->GetDesc(&ibDesc);
  void* idata = nullptr;
  ib->Lock(0, 0, &idata, 0);
  UINT faceCount = xmesh->GetNumFaces();
  UINT indexCount = faceCount * 3;
  mesh.indices.resize(indexCount);
  if (ibDesc.Format == D3DFMT_INDEX16) {
    // 16-bit 索引
    auto src = reinterpret_cast<uint16_t*>(idata);
    for (UINT f = 0; f < indexCount; ++f) {
      mesh.indices[f] = static_cast<uint32_t>(src[f]);
    }
  }
  else {
    // 32-bit 索引
    memcpy(
      mesh.indices.data(),          // 目標緩衝
      idata,                      // 來源緩衝
      indexCount * sizeof(uint32_t) // byte 數量
    );
  }
  //mesh.indices.resize(xmesh->GetNumFaces() * 3);
  //memcpy(mesh.indices.data(), idata, mesh.indices.size() * sizeof(uint32_t));
  ib->Unlock();
  ib->Release();

 

  // Cleanup
  if (materialBuffer) {
    materialBuffer->Release();
    materialBuffer = nullptr;
  }
  if (adjacency) {
    adjacency->Release();
    adjacency = nullptr;
  }
  xmesh->Release();
  
  ID3DXAnimationController* pAnimCtrl = nullptr;
  // 同檔案再呼叫 LoadMeshHierarchy 取得骨架
  FrameEx* rootFrame = nullptr;
  AllocateHierarchy alloc(dev);
  hr = D3DXLoadMeshHierarchyFromX(
    file.c_str(),
    D3DXMESH_MANAGED,
    dev,
    &alloc,
    nullptr,       // ID3DXLoadUserDataCallback*
    reinterpret_cast<D3DXFRAME**>(&rootFrame),
    &pAnimCtrl
  );
  if (FAILED(hr) || !rootFrame) {
    std::wcerr << L"Failed to load .x skeleton: " << file << std::endl;
    
    // 創建頂點和索引緩衝區 (即使沒有骨架也要創建)
    if (!mesh.CreateBuffers(dev)) {
      std::cerr << "Failed to create vertex/index buffers for mesh (no skeleton)" << std::endl;
      return false;
    }
    
    return true; // 只有 mesh 時也算成功, 如果沒有骨架，直接返回
  }

  // 找到名稱為 horse05 的 Frame
  FrameEx* pHorse05Frame = FindFrameByName(rootFrame, "x3ds_horse05");
  if (!pHorse05Frame) {
    // 沒找到
    printf("未找到 horse05\n");
    return false;
  }
  // 取出它的 MeshContainer（假設只有一個 container）
  MeshContainerEx* pContainer = static_cast<MeshContainerEx*>(pHorse05Frame->pMeshContainer);
  if (!pContainer || !pContainer->MeshData.pMesh) {
    printf("horse05 沒有 Mesh\n");
    return false;
  }

  // 現在你已經拿到 ID3DXMesh*：
  ID3DXMesh* pHorse05Mesh = pContainer->MeshData.pMesh;

  // 3. 計算所有 CombinedTransform（用第一個 frame 作為根，parentMat 設 Identity）
  D3DXMATRIX identity;
  D3DXMatrixIdentity(&identity);
  UpdateCombined(rootFrame, identity);

  // 4. 收集所有 FrameEx 並記錄父索引
  std::vector<FrameEx*> frames;
  std::vector<int>      parents;
  CollectFrames(rootFrame, frames, parents, -1);

  // 5. 填入 Skeleton.joints
  outSkel.joints.resize(frames.size());
  for (int i = 0; i < (int)frames.size(); ++i) {
    FrameEx* f = frames[i];
    auto& J = outSkel.joints[i];

    // 名稱與父索引
    J.name = f->Name ? f->Name : "";
    J.parentIndex = parents[i];

    // bind-pose inverse = CombinedTransform^-1
    // 先把 D3DXMATRIX 轉到 XMFLOAT4X4
    XMFLOAT4X4 m;
    memcpy(&m, &f->CombinedTransform, sizeof(m));
    XMMATRIX xm = XMLoadFloat4x4(&m);
    XMVECTOR det;
    XMMATRIX inv = XMMatrixInverse(&det, xm);
    XMStoreFloat4x4(&J.bindPoseInverse, inv);
  }

  // 6. 清除 frame tree
  alloc.DestroyFrame(rootFrame);

  // 創建頂點和索引緩衝區
  if (!mesh.CreateBuffers(dev)) {
    std::cerr << "Failed to create vertex/index buffers for mesh" << std::endl;
    return false;
  }

  return true;
}