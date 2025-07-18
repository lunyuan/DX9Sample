#pragma once

#include <d3dx9.h>       // ID3DXAllocateHierarchy, D3DXFRAME, D3DXMESHCONTAINER
#include <string>        // std::string
#include "XFileTypes.h"  // FrameEx, MeshContainerEx 定義

class AllocateHierarchy : public ID3DXAllocateHierarchy {
  IDirect3DDevice9* m_device;

public:
  explicit AllocateHierarchy(IDirect3DDevice9* device) noexcept;

  STDMETHOD(CreateFrame)(
    LPCSTR      Name,
    D3DXFRAME** ppNewFrame
    );

  STDMETHOD(CreateMeshContainer)(
    LPCSTR Name,
    const D3DXMESHDATA* pMeshData,
    const D3DXMATERIAL* pMaterials,
    const D3DXEFFECTINSTANCE* pEffectInstances,
    DWORD NumMaterials,
    const DWORD* pAdjacency,
    ID3DXSkinInfo* pSkinInfo,
    D3DXMESHCONTAINER** ppNewMeshContainer
    );

  STDMETHOD(DestroyFrame)(
    D3DXFRAME* pFrame
    );

  STDMETHOD(DestroyMeshContainer)(
    D3DXMESHCONTAINER* pMeshContainer
    );
};
