#pragma once
#include "SkinMesh.h"
#include "XFileTypes.h"
#include <stdexcept>
#include <DirectXMath.h>
#include <d3d9.h>

using namespace DirectX;

inline SkinMesh CreateSkinMesh(IDirect3DDevice9* device, MeshContainerEx* mc) {
  if (!device) throw std::invalid_argument("device is null");
  if (!mc || !mc->m_pMesh) throw std::invalid_argument("invalid mesh container");
  
  SkinMesh mesh;
  
  // Get mesh data from D3DXMesh
  ID3DXMesh* d3dMesh = mc->m_pMesh;
  
  // Get vertex count and copy vertices
  UINT numVerts = d3dMesh->GetNumVertices();
  mesh.vertices.resize(numVerts);
  
  // Lock vertex buffer and copy data
  IDirect3DVertexBuffer9* vb = nullptr;
  d3dMesh->GetVertexBuffer(&vb);
  void* vdata = nullptr;
  vb->Lock(0, 0, &vdata, D3DLOCK_READONLY);
  
  // Copy vertex data - assuming standard FVF
  DWORD fvf = d3dMesh->GetFVF();
  UINT stride = d3dMesh->GetNumBytesPerVertex();
  
  for (UINT i = 0; i < numVerts; ++i) {
    BYTE* src = reinterpret_cast<BYTE*>(vdata) + i * stride;
    Vertex& dst = mesh.vertices[i];
    
    // Position (always first in FVF)
    memcpy(&dst.pos, src, sizeof(XMFLOAT3));
    src += sizeof(XMFLOAT3);
    
    // Normal (if present)
    if (fvf & D3DFVF_NORMAL) {
      memcpy(&dst.norm, src, sizeof(XMFLOAT3));
      src += sizeof(XMFLOAT3);
    } else {
      dst.norm = XMFLOAT3(0, 1, 0); // default up
    }
    
    // Texture coordinates (if present)
    if (fvf & D3DFVF_TEX1) {
      memcpy(&dst.uv, src, sizeof(XMFLOAT2));
      src += sizeof(XMFLOAT2);
    } else {
      dst.uv = XMFLOAT2(0, 0);
    }
    
    // Set defaults for other fields
    dst.col = 0xFFFFFFFF;
    dst.spec = 0;
    dst.weights = XMFLOAT4(1, 0, 0, 0);
    memset(dst.boneIndices, 0, 4);
  }
  
  vb->Unlock();
  vb->Release();
  
  // Get index count and copy indices
  UINT numFaces = d3dMesh->GetNumFaces();
  UINT numIndices = numFaces * 3;
  mesh.indices.resize(numIndices);
  
  // Lock index buffer and copy data
  IDirect3DIndexBuffer9* ib = nullptr;
  d3dMesh->GetIndexBuffer(&ib);
  D3DINDEXBUFFER_DESC ibDesc;
  ib->GetDesc(&ibDesc);
  void* idata = nullptr;
  ib->Lock(0, 0, &idata, D3DLOCK_READONLY);
  
  if (ibDesc.Format == D3DFMT_INDEX16) {
    // 16-bit indices
    uint16_t* src = reinterpret_cast<uint16_t*>(idata);
    for (UINT i = 0; i < numIndices; ++i) {
      mesh.indices[i] = src[i];
    }
  } else {
    // 32-bit indices
    memcpy(mesh.indices.data(), idata, numIndices * sizeof(uint32_t));
  }
  
  ib->Unlock();
  ib->Release();
  
  // Copy materials
  if (mc->m_pMaterials && mc->NumMaterials > 0) {
    mesh.materials.resize(mc->NumMaterials);
    for (DWORD i = 0; i < mc->NumMaterials; ++i) {
      mesh.materials[i].mat = mc->m_pMaterials[i];
      if (i < mc->m_Textures.size()) {
        mesh.materials[i].tex = mc->m_Textures[i];
      }
    }
  }
  
  // Create GPU buffers
  if (!mesh.CreateBuffers(device)) {
    throw std::runtime_error("Failed to create vertex/index buffers");
  }
  
  return mesh;
}
