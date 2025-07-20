#pragma once
#include "IInputListener.h" 
#include <DirectXMath.h>
#include <d3d9.h>
#include <memory>
using namespace DirectX;

/// <summary>攝影機軌道／平移／推拉 控制器</summary>
struct ICameraController : IInputListener {
  virtual ~ICameraController() = default;

  /// 每幀更新攝影機動畫
  virtual void Update(float deltaTime) = 0;

  /// 取得最後計算好的 View 矩陣
  virtual XMMATRIX GetViewMatrix() const = 0;
  /// 取得最後計算好的投影矩陣
  virtual XMMATRIX GetProjMatrix(float aspect) const = 0;
};

/// <summary>Factory 函式：建立預設實作的 CameraController。</summary>
std::unique_ptr<ICameraController> CreateCameraController(IDirect3DDevice9* device, int width, int height);
