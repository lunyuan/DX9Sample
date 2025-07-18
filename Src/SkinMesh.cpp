#define NOMINMAX
#include "SkinMesh.h"
#include <d3dx9.h>
#include <iostream>
#include <DirectXMath.h>

// 全域或成員變數：只建立一次
static IDirect3DVertexDeclaration9* g_pDecl = nullptr;

void InitVertexDecl(IDirect3DDevice9* dev) {
  if (g_pDecl) return;
  D3DVERTEXELEMENT9 decl[] = {
    {0,   0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0,  12,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
    {0,  24,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
    {0,  28,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    1},
    {0,  32,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0,  40,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
    {0,  56,  D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
    // pack boneIndices as D3DCOLOR (4x8bit), or use D3DDECLTYPE_SHORT4 
    D3DDECL_END()
  };
  dev->CreateVertexDeclaration(decl, &g_pDecl);
}

bool  SkinMesh::CreateBuffers(IDirect3DDevice9* dev) {
  ReleaseBuffers();

  // VertexBuffer
  UINT vbSize = UINT(vertices.size() * sizeof(Vertex));
  HRESULT hr = dev->CreateVertexBuffer(
    vbSize,
    D3DUSAGE_WRITEONLY,
    0,                // 使用 Vertex Declaration 而非 FVF，就傳 0
    D3DPOOL_MANAGED,
    &vb,
    nullptr
  );
  if (FAILED(hr) || !vb) {
    std::cerr << "CreateVertexBuffer 失敗, size=" << vbSize << std::endl;
    return false;
  }
  // 填頂點資料
  void* pV = nullptr;
  if (SUCCEEDED(vb->Lock(0, vbSize, &pV, 0))) {
    memcpy(pV, vertices.data(), vbSize);
    vb->Unlock();
  }
  else {
    std::cerr << "VertexBuffer Lock 失敗\n";
    vb->Release();
    vb = nullptr;
    return false;
  }

  // 3. 建立 IndexBuffer(32 - bit)
  UINT ibSize = UINT(indices.size() * sizeof(uint32_t));
  hr = dev->CreateIndexBuffer(
    ibSize,
    D3DUSAGE_WRITEONLY,
    D3DFMT_INDEX32,
    D3DPOOL_MANAGED,
    &ib,
    nullptr
  );
  if (FAILED(hr) || !ib) {
    std::cerr << "CreateIndexBuffer 失敗, size=" << ibSize << std::endl;
    ReleaseBuffers();
    return false;
  }

  // 填索引資料
  void* pI = nullptr;
  if (SUCCEEDED(ib->Lock(0, ibSize, &pI, 0))) {
    memcpy(pI, indices.data(), ibSize);
    ib->Unlock();
  }
  else {
    std::cerr << "IndexBuffer Lock 失敗\n";
    ReleaseBuffers();
    return false;
  }

  InitVertexDecl(dev);
  ReleaseBuffers();
  return true;
}

void SkinMesh::LoadMaterials(IDirect3DDevice9* dev, ID3DXBuffer* materialBuffer, DWORD numMaterials) {
  auto mats = reinterpret_cast<D3DXMATERIAL*>(materialBuffer->GetBufferPointer());
  materials.resize(numMaterials);
  for (DWORD i = 0; i < numMaterials; ++i) {
    // 1) 複製 D3DMATERIAL9
    materials[i].mat = mats[i].MatD3D;
    // 設定 α 混合參數（若需要）
    materials[i].mat.Power = mats[i].MatD3D.Power;
    materials[i].mat.Diffuse.a = mats[i].MatD3D.Diffuse.a;
    materials[i].mat.Specular.a = mats[i].MatD3D.Specular.a;
    materials[i].mat.Ambient = materials[i].mat.Diffuse; // 確保環境光與漫反射一致
    // 2) 載入貼圖（如果檔名非空）
    if (mats[i].pTextureFilename && mats[i].pTextureFilename[0] != '\0') {
      HRESULT hr = D3DXCreateTextureFromFileA(
        dev,
        mats[i].pTextureFilename,
        &materials[i].tex
      );
      if (FAILED(hr)) {
        std::cerr << "Warning: 載入貼圖失敗: "
          << mats[i].pTextureFilename << "\n";
        materials[i].tex = nullptr;
      }
    }
    else {
      materials[i].tex = nullptr;
    }
  }
}

void SkinMesh::ReleaseBuffers() {
  if (vb) { vb->Release();      vb = nullptr; }
  if (ib) { ib->Release();      ib = nullptr; }
  if (texture) { texture->Release(); texture = nullptr; }
}

void SkinMesh::SetTexture(IDirect3DDevice9* dev, const std::string& file) {
  // 釋放舊貼圖
  if (texture) {
    texture->Release();
    texture = nullptr;
  }
  // 從檔案建立新貼圖
  HRESULT hr = D3DXCreateTextureFromFileA(dev, file.c_str(), &texture);
  if (FAILED(hr)) {
    std::cerr << "SetTexture 無法載入貼圖: " << file << std::endl;
  }
}

void SkinMesh::Draw(IDirect3DDevice9* dev) {
  // 1. 世界矩陣（如果你已在外面設定就不用重設）
  //D3DMATRIX matW;
  //XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&matW),
  //  DirectX::XMMatrixIdentity());
  //dev->SetTransform(D3DTS_WORLD, &matW);

  // 2. 關閉光照（若全域已關可跳過）
  //dev->SetRenderState(D3DRS_LIGHTING, FALSE);

  // 3. 綁定貼圖與取樣器
  dev->SetTexture(0, texture);
  dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

  // 4. 明確設定固定管線的貼圖運算方式
  dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

  // Alpha 同理，如果需要
  dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

  dev->SetVertexDeclaration(g_pDecl);

  //// Debug 用：切到線框模式
  //dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  //// 關掉背面剔除
  //dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  // 5. 綁定頂點、索引緩衝並繪製
  //dev->SetStreamSource(0, vb, 0, sizeof(Vertex));
  //dev->SetIndices(ib);
  //dev->DrawIndexedPrimitive(
  //  D3DPT_TRIANGLELIST,
  //  0, 0,
  //  UINT(vertices.size()),
  //  0,
  //  UINT(indices.size() / 3)
  //);
  dev->SetStreamSource(0, vb, 0, 0);
  UINT numVerts = static_cast<UINT>(vertices.size());
  UINT indexCount = static_cast<UINT>(indices.size());
  UINT primCount  = indexCount / 3;
  // 1. 資料量檢查
  if (numVerts == 0 || primCount == 0) {
    std::cerr << "頂點或三角形數為 0\n";
    return;
  }
  // 2. 索引越界檢查
  //UINT maxIdx = 0;
  //for (auto idx : indices) maxIdx = max(maxIdx, idx);
  //if (maxIdx >= numVerts) {
  //  std::cerr << "索引越界：maxIdx=" << maxIdx
  //    << " numVerts=" << numVerts << "\n";
  //  return;
  //}

  //// 找出最大索引
  //UINT maxIdx = 0;
  //for (UINT i = 0; i < indices.size(); ++i) {
  //  maxIdx = std::max(maxIdx, indices[i]);
  //}
  //std::cerr
  //  << "頂點數: " << numVerts
  //  << "，最大索引: " << maxIdx << "\n";

  //// 如果超出範圍，這裡就不呼叫繪製，避免崩潰
  //if (maxIdx >= numVerts) {
  //  std::cerr
  //    << "索引越界！maxIdx=" << maxIdx
  //    << " >= vertices.size()=" << numVerts << "\n";
  //  return;
  //}
  HRESULT hr;
  if (materials.size() > 0) {
    for (DWORD i = 0; i < materials.size(); ++i) {
      // 設定材質
      dev->SetMaterial(&materials[i].mat);
      // 綁定貼圖
      dev->SetTexture(0, materials[i].tex);
      // 畫出該 subset
      hr = dev->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST,                                   // PrimitiveType
        0,                                                     // MinVertexIndex
        numVerts,                    // NumVertices
        primCount,                 // PrimitiveCount (三角形數)
        indices.data(),                                        // pIndexData
        D3DFMT_INDEX32,                                        // IndexDataFormat
        vertices.data(),                                       // pVertexStreamZeroData
        sizeof(Vertex)                                         // VertexStreamZeroStride
      );
    }
  }
  if(texture) {
    dev->SetTexture(0, texture);
    // 如果沒有材質，就直接畫全部
    hr = dev->DrawIndexedPrimitiveUP(
      D3DPT_TRIANGLELIST,                                   // PrimitiveType
      0,                                                     // MinVertexIndex
      numVerts,                    // NumVertices
      primCount,                 // PrimitiveCount (三角形數)
      indices.data(),                                        // pIndexData
      D3DFMT_INDEX32,                                        // IndexDataFormat
      vertices.data(),                                       // pVertexStreamZeroData
      sizeof(Vertex)                                         // VertexStreamZeroStride
    );
  }
  if (FAILED(hr)) {
    std::cerr << "DrawIndexedPrimitiveUP 失敗，HRESULT=0x"
      << std::hex << hr << std::dec << "\n";
  }
  //HRESULT hr = dev->DrawIndexedPrimitiveUP(
  //  D3DPT_TRIANGLELIST,                                   // PrimitiveType
  //  0,                                                     // MinVertexIndex
  //  numVerts,                    // NumVertices
  //  primCount,                 // PrimitiveCount (三角形數)
  //  indices.data(),                                        // pIndexData
  //  D3DFMT_INDEX32,                                        // IndexDataFormat
  //  vertices.data(),                                       // pVertexStreamZeroData
  //  sizeof(Vertex)                                         // VertexStreamZeroStride
  //);
  //if (FAILED(hr)) {
  //  std::cerr << "DrawIndexedPrimitiveUP 失敗，HRESULT=0x"
  //    << std::hex << hr << std::dec << "\n";
  //}

  //dev->DrawIndexedPrimitiveUP(
  //  D3DPT_TRIANGLELIST,           // primitive type
  //  0,                            // MinVertexIndex
  //  static_cast<UINT>(vertices.size()), // NumVertices
  //  static_cast<UINT>(indices.size() / 3), // PrimitiveCount
  //  indices.data(),               // pIndexData
  //  D3DFMT_INDEX32,               // index format
  //  vertices.data(),              // pVertexStreamZeroData
  //  sizeof(Vertex)                // VertexStreamZeroStride
  //);

}