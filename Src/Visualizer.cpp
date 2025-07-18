#include "Visualizer.h"
using namespace DX;


void Visualizer::DrawJoints(IDirect3DDevice9* dev,
  const Skeleton& skel, const XMFLOAT4X4* globals) {
  struct V { float x, y, z; D3DCOLOR color; };
  std::vector<V> lines;
  for (size_t i = 0; i < skel.joints.size(); ++i) {
    int p = skel.joints[i].parentIndex;
    if (p < 0) continue;
    // 父子關節位置
    XMFLOAT4X4 m0 = globals[p];
    XMFLOAT4X4 m1 = globals[i];
    D3DCOLOR col = D3DCOLOR_XRGB(0, 255, 0);
    lines.push_back({ m0._41, m0._42, m0._43, col });
    lines.push_back({ m1._41, m1._42, m1._43, col });
  }
  dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
  dev->DrawPrimitiveUP(D3DPT_LINELIST,
    (UINT)lines.size() / 2,
    lines.data(),
    sizeof(V));
}

void Visualizer::DrawWeights(IDirect3DDevice9* dev,
  const SkinMesh& mesh, const Skeleton& skel, const XMFLOAT4X4* globals) {
  struct V { float x, y, z; D3DCOLOR color; };
  std::vector<V> lines;
  for (auto& v : mesh.vertices) {
    // 找到影響最大骨骼
    int bi = 0; float bw = v.weights.x;
    if (v.weights.y > bw) { bw = v.weights.y; bi = 1; }
    if (v.weights.z > bw) { bw = v.weights.z; bi = 2; }
    if (v.weights.w > bw) { bw = v.weights.w; bi = 2; }
    // 原始位置
    XMVECTOR pos = XMVectorSet(v.pos.x, v.pos.y, v.pos.z, 1);
    // 變換後位置
    XMMATRIX wm = XMLoadFloat4x4(&globals[bi]);
    XMVECTOR wp = XMVector3Transform(pos, wm);
    XMFLOAT3 wp3; XMStoreFloat3(&wp3, wp);
    // 顏色漸層
    int cval = (int)(bw * 255);
    D3DCOLOR col = D3DCOLOR_XRGB(cval, 0, 255 - cval);
    lines.push_back({ v.pos.x,  v.pos.y,  v.pos.z,  col });
    lines.push_back({ wp3.x, wp3.y, wp3.z, col });
  }
  dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
  dev->DrawPrimitiveUP(D3DPT_LINELIST,
    (UINT)lines.size() / 2,
    lines.data(),
    sizeof(V));
}