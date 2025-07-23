#include "Scene3D.h"
#include <d3dx9.h>
#include "Src/SkinMesh.h"

// Factory 函式實作
std::unique_ptr<IScene3D> CreateScene3D() {
  return std::make_unique<Scene3D>();
}
HRESULT STDMETHODCALLTYPE Scene3D::Init(IDirect3DDevice9* dev,
  ILightManager* lightMgr,
  const std::wstring& meshFile,
  const std::wstring& texFile) {
  if (!dev || meshFile.empty() || texFile.empty()) return E_INVALIDARG;
  HRESULT hr = D3DXLoadMeshFromX(meshFile.c_str(), D3DXMESH_SYSTEMMEM,
    dev, nullptr, nullptr, nullptr, nullptr, &mesh_);
  if (FAILED(hr)) return hr;
  hr = D3DXCreateTextureFromFile(dev, texFile.c_str(), &tex_);
  if (FAILED(hr)) return hr;
  /*hr = D3DXCreateEffectFromFile(dev, L"ModelLit.fx",
    nullptr, nullptr, 0, nullptr, &fx_, nullptr);*/

  // 儲存 lightMgr
  lightMgr_ = lightMgr;

  // **Cache parameter handles once**
  if (fx_) {
    hView_ = fx_->GetParameterByName(nullptr, "g_View");
    hProj_ = fx_->GetParameterByName(nullptr, "g_Proj");
  }
  
  return S_OK;
}

HRESULT STDMETHODCALLTYPE Scene3D::Render(IDirect3DDevice9* dev,
  const XMMATRIX& view,  const XMMATRIX& proj, IUIManager* uiManager)
{
  if (!dev || !mesh_) return E_POINTER;
  
  // 設定變換矩陣
  D3DXMATRIX worldMatrix, viewMatrix, projMatrix;
  D3DXMatrixIdentity(&worldMatrix);
  
  // 轉換 DirectXMath 矩陣為 D3DX 矩陣
  XMFLOAT4X4 viewF, projF;
  XMStoreFloat4x4(&viewF, view);
  XMStoreFloat4x4(&projF, proj);
  memcpy(&viewMatrix, &viewF, sizeof(D3DXMATRIX));
  memcpy(&projMatrix, &projF, sizeof(D3DXMATRIX));
  
  // 設定變換矩陣到裝置
  dev->SetTransform(D3DTS_WORLD, &worldMatrix);
  dev->SetTransform(D3DTS_VIEW, &viewMatrix);
  dev->SetTransform(D3DTS_PROJECTION, &projMatrix);
  
  // 應用光照
  if (lightMgr_) {
    lightMgr_->ApplyAll(dev);
  }
  
  // 設定簡單的白色材質
  D3DMATERIAL9 material;
  ZeroMemory(&material, sizeof(D3DMATERIAL9));
  material.Diffuse.r = material.Diffuse.g = material.Diffuse.b = material.Diffuse.a = 1.0f;
  material.Ambient.r = material.Ambient.g = material.Ambient.b = material.Ambient.a = 1.0f;
  dev->SetMaterial(&material);
  
  // 設定材質
  if (tex_) {
    dev->SetTexture(0, tex_.Get());
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  }
  
  // 如果有 effect shader 則使用，否則使用固定功能管線
  if (fx_) {
    // Shader 渲染路徑
    D3DXMATRIX WVP = worldMatrix * viewMatrix * projMatrix;
    fx_->SetMatrix("g_WVP", &WVP);
    D3DXVECTOR4 lightDir(0.577f, -0.577f, 0.577f, 0.0f);
    fx_->SetVector("g_LightDir", &lightDir);
    fx_->SetTexture("g_DiffuseTex", tex_.Get());
    fx_->SetTechnique(fx_->GetTechniqueByName("Tech_ModelLit"));
    
    UINT passes = 0;
    fx_->Begin(&passes, 0);
    for (UINT i = 0; i < passes; ++i) {
      fx_->BeginPass(i);
      
      // 嘗試繪製多個子集（多匹馬）
      for (DWORD j = 0; j < 20; ++j) {
        HRESULT hr = mesh_->DrawSubset(j);
        if (FAILED(hr)) break;  // 如果失敗就停止
      }
      
      fx_->EndPass();
    }
    fx_->End();
  } else {
    // 固定功能管線渲染路徑
    dev->SetRenderState(D3DRS_LIGHTING, TRUE);
    dev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    
    // 嘗試繪製多個子集（多匹馬）
    for (DWORD i = 0; i < 20; ++i) {
      HRESULT hr = mesh_->DrawSubset(i);
      if (FAILED(hr)) break;  // 如果失敗就停止
    }
  }
  
  // 渲染UI (在3D場景之後，確保UI在最上層)
  if (uiManager) {
    uiManager->Render(dev);
  }
  
  return S_OK;
}
