#pragma once
#include "SkinMesh.h"
#include "XFileTypes.h"
#include <stdexcept>

inline SkinMesh CreateSkinMesh(IDirect3DDevice9* device, MeshContainerEx* mc) {
  if (!device) throw std::invalid_argument("device is null");
  if (!mc || !mc->m_pMesh) throw std::invalid_argument("invalid mesh container");
  SkinMesh mesh;
  return mesh;
}
