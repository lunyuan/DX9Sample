#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include <DirectXMath.h>

using namespace DirectX;

//struct Vertex {
//  float x, y, z;
//  float nx, ny, nz;
//  float u, v;
//  uint8_t boneIndices[4];
//  float   boneWeights[4];
//};

struct Vertex {
  XMFLOAT3    pos;        // 位置
  XMFLOAT3    norm;       // 法線
  D3DCOLOR    col;        // 頂點色
  D3DCOLOR    spec;       // 鏡面色
  XMFLOAT2    uv;         // UV
  XMFLOAT4    weights;    // 骨骼權重
  uint8_t     boneIndices[4];// 骨骼索引
};

struct VertexSimple {
  XMFLOAT3    pos;    // 位置
  float       rhw;    // 齊次倒數（若使用 XYZRHW 模式）
  XMFLOAT3    norm;   // 法線
  D3DCOLOR    color;  // 頂點顏色
  XMFLOAT2    uv;     // UV 座標
};

struct Material {
  D3DMATERIAL9        mat;      // 漫反射／鏡面／環境等材質參數
  IDirect3DTexture9* tex = nullptr;  // 對應的貼圖 (若有)
};


typedef struct _ISkinMesh {
  std::string Name = {};
  std::vector<_ISkinMesh> Sibling = {};
  std::vector<_ISkinMesh> Child = {};
} ISkinMesh;

// 使用 SkinMesh 避免與 D3DXMesh 衝突
class SkinMesh : public ISkinMesh {
public:
  std::vector<Vertex> vertices = {};
  std::vector<uint32_t> indices = {};
  std::vector<Material> materials = {}; // 支援多材質

  IDirect3DVertexBuffer9* vb = nullptr;
  IDirect3DIndexBuffer9* ib = nullptr;
  IDirect3DTexture9* texture = nullptr;

  bool CreateBuffers(IDirect3DDevice9* dev);
  void LoadMaterials(IDirect3DDevice9* dev, ID3DXBuffer* materialBuffer, DWORD numMaterials);
  void SetTexture(IDirect3DDevice9* dev, const std::string& file);
  void Draw(IDirect3DDevice9* dev);
  /// 釋放緩衝資源
  void ReleaseBuffers();
};