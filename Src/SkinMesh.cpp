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
  HRESULT hr = dev->CreateVertexDeclaration(decl, &g_pDecl);
  if (FAILED(hr)) {
    OutputDebugStringA(("Failed to create vertex declaration! HRESULT=0x" + 
                       std::to_string(hr) + "\n").c_str());
  } else {
    OutputDebugStringA("Vertex declaration created successfully\n");
  }
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
  return true;  // Fixed: Don't release buffers after creating them!
}

void SkinMesh::LoadMaterials(IDirect3DDevice9* dev, ID3DXBuffer* materialBuffer, DWORD numMaterials) {
  auto mats = reinterpret_cast<D3DXMATERIAL*>(materialBuffer->GetBufferPointer());
  materials.resize(numMaterials);
  
  OutputDebugStringA(("SkinMesh::LoadMaterials - Loading " + std::to_string(numMaterials) + " materials\n").c_str());
  
  for (DWORD i = 0; i < numMaterials; ++i) {
    // 1) 複製 D3DMATERIAL9
    materials[i].mat = mats[i].MatD3D;
    // 設定 α 混合參數（若需要）
    materials[i].mat.Power = mats[i].MatD3D.Power;
    materials[i].mat.Diffuse.a = mats[i].MatD3D.Diffuse.a;
    materials[i].mat.Specular.a = mats[i].MatD3D.Specular.a;
    materials[i].mat.Ambient = materials[i].mat.Diffuse; // 確保環境光與漫反射一致
    
    // 調試：輸出材質屬性
    char matDebug[256];
    sprintf_s(matDebug, "  Material %d: Diffuse=(%.2f,%.2f,%.2f,%.2f)\n", 
              i, materials[i].mat.Diffuse.r, materials[i].mat.Diffuse.g, 
              materials[i].mat.Diffuse.b, materials[i].mat.Diffuse.a);
    OutputDebugStringA(matDebug);
    
    // 2) 載入貼圖（如果檔名非空）
    if (mats[i].pTextureFilename && mats[i].pTextureFilename[0] != '\0') {
      OutputDebugStringA(("  Loading texture: " + std::string(mats[i].pTextureFilename) + "\n").c_str());
      
      HRESULT hr = D3DXCreateTextureFromFileA(
        dev,
        mats[i].pTextureFilename,
        &materials[i].tex
      );
      if (FAILED(hr)) {
        std::cerr << "Warning: 載入貼圖失敗: "
          << mats[i].pTextureFilename << "\n";
        OutputDebugStringA(("  Failed to load texture! HRESULT=0x" + 
                           std::to_string(hr) + "\n").c_str());
        materials[i].tex = nullptr;
      } else {
        OutputDebugStringA(("  Texture loaded successfully! ptr=" + 
                           std::to_string((size_t)materials[i].tex) + "\n").c_str());
      }
    }
    else {
      OutputDebugStringA("  No texture filename specified\n");
      materials[i].tex = nullptr;
    }
  }
  
  OutputDebugStringA("SkinMesh::LoadMaterials - Finished loading materials\n");
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
  
  // 設置第二層紋理為禁用
  dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

  dev->SetVertexDeclaration(g_pDecl);

  //// Debug 用：切到線框模式
  //dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  //// 關掉背面剔除
  //dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  // 5. 綁定頂點、索引緩衝並繪製
  dev->SetStreamSource(0, vb, 0, sizeof(Vertex));
  dev->SetIndices(ib);
  
  UINT numVerts = static_cast<UINT>(vertices.size());
  UINT indexCount = static_cast<UINT>(indices.size());
  UINT primCount  = indexCount / 3;
  // 1. 資料量檢查
  if (numVerts == 0 || primCount == 0) {
    std::cerr << "頂點或三角形數為 0\n";
    return;
  }
  
  // 調試：檢查頂點的紋理座標
  static int uvCheckCount = 0;
  if (uvCheckCount++ % 600 == 0 && vertices.size() > 0) { // 每10秒檢查一次
    bool hasValidUV = false;
    for (size_t i = 0; i < std::min(vertices.size(), size_t(10)); ++i) {
      if (vertices[i].uv.x != 0.0f || vertices[i].uv.y != 0.0f) {
        hasValidUV = true;
        break;
      }
    }
    char debugMsg[256];
    sprintf_s(debugMsg, "UV Check: %s valid UV coords (sample: uv[0]=(%.3f,%.3f))\n", 
              hasValidUV ? "Has" : "No", vertices[0].uv.x, vertices[0].uv.y);
    OutputDebugStringA(debugMsg);
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
  
  // 如果有通過 SetTexture 設定的紋理，優先使用它
  if (texture) {
    // 使用覆蓋的紋理
    dev->SetTexture(0, texture);
    
    // 如果有材質，只使用材質屬性（不使用材質的紋理）
    if (materials.size() > 0) {
      // 只設定第一個材質的屬性（光照等）
      dev->SetMaterial(&materials[0].mat);
    }
    
    // 繪製整個網格
    // 使用 DrawIndexedPrimitive 而不是 DrawIndexedPrimitiveUP
    hr = dev->DrawIndexedPrimitive(
      D3DPT_TRIANGLELIST,
      0,                    // BaseVertexIndex
      0,                    // MinVertexIndex
      numVerts,             // NumVertices
      0,                    // StartIndex
      primCount             // PrimitiveCount
    );
  }
  else if (materials.size() > 0) {
    // 沒有覆蓋紋理，使用材質中的紋理
    // 注意：這裡簡化處理，對每個材質都繪製整個網格
    // 實際上應該根據材質分組繪製不同的面，但目前我們沒有subset資訊
    
    // 如果只有一個材質，直接繪製
    if (materials.size() == 1) {
      dev->SetMaterial(&materials[0].mat);
      dev->SetTexture(0, materials[0].tex);
      
      static int drawCount = 0;
      if (drawCount++ % 300 == 0) {
        // 檢查當前的紋理狀態
        IDirect3DBaseTexture9* currentTex = nullptr;
        dev->GetTexture(0, &currentTex);
        
        OutputDebugStringA(("Drawing with 1 material, tex=" + 
                           std::to_string((size_t)materials[0].tex) + 
                           ", current tex=" + std::to_string((size_t)currentTex) + "\n").c_str());
        
        if (currentTex) currentTex->Release();
      }
      
      // 使用 DrawIndexedPrimitive 而不是 DrawIndexedPrimitiveUP
      hr = dev->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0,                    // BaseVertexIndex
        0,                    // MinVertexIndex
        numVerts,             // NumVertices
        0,                    // StartIndex
        primCount             // PrimitiveCount
      );
    } else {
      // 多個材質：暫時只使用第一個材質
      // TODO: 需要實作材質分組（subset）功能
      dev->SetMaterial(&materials[0].mat);
      dev->SetTexture(0, materials[0].tex);
      
      static int multiDrawCount = 0;
      if (multiDrawCount++ % 300 == 0) {
        OutputDebugStringA(("Drawing with " + std::to_string(materials.size()) + 
                           " materials, using first tex=" + 
                           std::to_string((size_t)materials[0].tex) + "\n").c_str());
      }
      
      // 使用 DrawIndexedPrimitive 而不是 DrawIndexedPrimitiveUP
      hr = dev->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0,                    // BaseVertexIndex
        0,                    // MinVertexIndex
        numVerts,             // NumVertices
        0,                    // StartIndex
        primCount             // PrimitiveCount
      );
    }
  }
  else {
    // 沒有紋理也沒有材質，直接繪製
    dev->SetTexture(0, nullptr);
    // 使用 DrawIndexedPrimitive 而不是 DrawIndexedPrimitiveUP
    hr = dev->DrawIndexedPrimitive(
      D3DPT_TRIANGLELIST,
      0,                    // BaseVertexIndex
      0,                    // MinVertexIndex
      numVerts,             // NumVertices
      0,                    // StartIndex
      primCount             // PrimitiveCount
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