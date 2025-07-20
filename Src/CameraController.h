#pragma once
#include "ICameraController.h"
#include "IInputListener.h"
#include <d3d9.h>
#include <algorithm>

class CameraController : public ICameraController {
public:
  CameraController(IDirect3DDevice9* dev, int width, int height);
  ~CameraController();
  bool HandleMessage(const MSG& msg) override;
  void Update(float deltaTime) override;
  XMMATRIX GetViewMatrix() const override;
  XMMATRIX GetProjMatrix(float aspect) const override;

private:
  int m_width = 0;
  int m_height = 0;
  IDirect3DDevice9* m_dev;
  // 狀態旗標
  bool   m_orbiting = false;
  bool   m_panning = false;
  bool   m_dollying = false;
  POINT  m_lastMouse = {};

  
  // 目標與當前參數 角度與距離
  float  m_targetYaw = 0, m_currentYaw = 0;
  float  m_targetPitch = 0, m_currentPitch = 0;
  float  m_targetDist = 10, m_currentDist = 10;
  XMVECTOR m_targetAt = XMVectorZero();
  XMVECTOR m_currentAt = XMVectorZero();

  // 常數速度
  static constexpr float kSmoothing = 0.1f;
  static constexpr float kPanSpeed = 0.01f;
  static constexpr float kDragZoomSpd = 0.005f;
  static constexpr float kZoomSpeed = 0.001f;
  static constexpr float kMinDist = 2.0f;
  static constexpr float kMaxDist = 50.0f;

  // 每幀或初始化時呼叫
  void SetupCamera();
  void SetupMatrices();
};
