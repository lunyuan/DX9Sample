#pragma once
#include <d3d9.h>
#include <DirectXMath.h>
#include "Skeleton.h"
#include "SkinMesh.h"
namespace DX = DirectX;

struct DebugVertex { float x, y, z; D3DCOLOR color; };

class Visualizer {
public:
  static void DrawJoints(IDirect3DDevice9* dev,
    const Skeleton& skel,
    const DX::XMFLOAT4X4* globals);
  static void DrawWeights(IDirect3DDevice9* dev,
    const SkinMesh& mesh,
    const Skeleton& skel,
    const DX::XMFLOAT4X4* globals);
};