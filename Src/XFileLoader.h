#pragma once
#include <string>
#include <d3dx9.h>
#include "SkinMesh.h"
#include "Skeleton.h"

class XFileLoader {
public:
  static bool Load(const std::wstring& file, IDirect3DDevice9* dev, SkinMesh& mesh, Skeleton& outSkel);
};