#include "CameraController.h"
#include <windowsx.h> // GET_WHEEL_DELTA_WPARAM
#include <DirectXMath.h>
using namespace DirectX;

// Factory 函式實作
std::unique_ptr<ICameraController> CreateCameraController(IDirect3DDevice9* device, int width, int height) {
  return std::make_unique<CameraController>(device, width, height);
}

CameraController::CameraController(IDirect3DDevice9* dev, int width, int height) {
  m_dev = dev;
  m_dev->AddRef();
  m_width = width;
  m_height = height;
  // 初始參數（可自訂）
  m_currentDist = m_targetDist = 10.0f;
}
CameraController::~CameraController() {
  if (m_dev) {
    m_dev->Release();
    m_dev = nullptr;
  }
}
bool CameraController::HandleMessage(const MSG& msg) {
  switch (msg.message) {
  case WM_LBUTTONDOWN:
    // Camera left button down
    m_orbiting = true;
    m_lastMouse.x = LOWORD(msg.lParam);
    m_lastMouse.y = HIWORD(msg.lParam);
    SetCapture(msg.hwnd);
    return true;

  case WM_MBUTTONDOWN:
    m_panning = true;
    m_lastMouse.x = LOWORD(msg.lParam);
    m_lastMouse.y = HIWORD(msg.lParam);
    SetCapture(msg.hwnd);
    return true;

  case WM_RBUTTONDOWN:
    m_dollying = true;
    m_lastMouse.x = LOWORD(msg.lParam);
    m_lastMouse.y = HIWORD(msg.lParam);
    SetCapture(msg.hwnd);
    return true;

  case WM_LBUTTONUP:
    m_orbiting = false;
    ReleaseCapture();
    return true;

  case WM_MBUTTONUP:
    m_panning = false;
    ReleaseCapture();
    return true;

  case WM_RBUTTONUP:
    m_dollying = false;
    ReleaseCapture();
    return true;

  case WM_MOUSEMOVE: {
    // Only process mouse movement if we're actively in a camera operation
    if (!m_orbiting && !m_panning && !m_dollying) {
      return false; // Let other components handle it
    }
    
    // Camera mouse move processing
    POINT cur{ GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam) };
    float dx = float(cur.x - m_lastMouse.x);
    float dy = float(cur.y - m_lastMouse.y);

    if (m_orbiting) {
      // Orbit: 調整目標 Yaw/Pitch
      m_targetYaw += dx * 0.005f;
      m_targetPitch += dy * 0.005f;
      // 限制 Pitch
      const float lim = XM_PIDIV2 - 0.01f;
      m_targetPitch = std::clamp(m_targetPitch, -lim, lim);
    }
    else if (m_panning) {
      // Pan: 沿著攝影機的 Right/Up 平移目標點
      // 先計算方向向量
      XMVECTOR dir = XMVectorSet(
        cosf(m_currentPitch) * sinf(m_currentYaw),
        sinf(m_currentPitch),
        cosf(m_currentPitch) * cosf(m_currentYaw),
        0
      );
      XMVECTOR up = XMVectorSet(0, 1, 0, 0);
      XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
      XMVECTOR realUp = XMVector3Normalize(XMVector3Cross(dir, right));
      // 根據滑鼠位移平移
      m_targetAt += (-right * dx + realUp * dy) * (kPanSpeed * m_currentDist);
    }
    else if (m_dollying) {
      // Dolly: 右鍵上下拖控制縮放
      m_targetDist = std::clamp(m_targetDist - dy * kDragZoomSpd, kMinDist, kMaxDist);
    }

    m_lastMouse = cur;
    return true;
  }

  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
    m_targetDist = std::clamp(m_targetDist - delta * kZoomSpeed, kMinDist, kMaxDist);
    return true;
  }

  case WM_KEYDOWN:
    switch (msg.wParam)
    {
    case 'F': // Focus: 把 g_targetAt 設為世界原點，可改成你欲聚焦物件的位置
      m_targetAt = XMVectorZero();
      m_targetDist = m_currentDist = 10.0f;
      m_targetYaw = m_currentYaw = 0.0f;
      m_targetPitch = m_currentPitch = 0.0f;
      return true;
    case VK_ADD:
      m_targetDist = std::clamp(m_targetDist - 0.5f, kMinDist, kMaxDist);
      return true;
    case VK_SUBTRACT:
      m_targetDist = std::clamp(m_targetDist + 0.5f, kMinDist, kMaxDist);
      return true;
    }
  case WM_DESTROY:
    PostQuitMessage(0);
    return true;
  }
  return false; // 沒處理的訊息交給 DefWindowProc
}


void CameraController::SetupCamera()
{
  // 1. 平滑插值 Yaw/Pitch/Dist/At
  m_currentYaw += (m_targetYaw - m_currentYaw) * kSmoothing;
  m_currentPitch += (m_targetPitch -m_currentPitch) * kSmoothing;
  m_currentDist += (m_targetDist - m_currentDist) * kSmoothing;
  m_currentAt = XMVectorLerp(m_currentAt, m_targetAt, kSmoothing);

  // 2. 計算攝影機位置
  XMVECTOR dir = XMVectorSet(
    cosf(m_currentPitch) * sinf(m_currentYaw),
    sinf(m_currentPitch),
    cosf(m_currentPitch) * cosf(m_currentYaw),
    0.0f
  );
  XMVECTOR camPos = m_currentAt - dir * m_currentDist;

  // 3. 建立 View 與 Projection
  XMMATRIX view = XMMatrixLookAtLH(camPos, m_currentAt, XMVectorSet(0, 1, 0, 0));
  XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,
    float(m_width) / float(m_height), 0.1f, 100.0f);

  // 4. 存成 D3DMATRIX 並設定
  XMFLOAT4X4 vf, pf;
  XMStoreFloat4x4(&vf, view);
  XMStoreFloat4x4(&pf, proj);
  m_dev->SetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(&vf));
  m_dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(&pf));

  // （如有需要也可在此重設 World）
}

// 每幀或初始化時呼叫
void CameraController::SetupMatrices()
{
  // 1. World：旋轉 + 平移
  float angle = XMConvertToRadians(30.0f);
  XMMATRIX rot = XMMatrixRotationY(angle);
  XMMATRIX trans = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
  XMMATRIX world = rot * trans;

  // 2. View：相機在 (0,2,-5)，看向 (0,1,0)，上方為 Y 軸
  XMVECTOR eye = XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f);
  XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMMATRIX view = XMMatrixLookAtLH(eye, at, up);

  float aspect = float(m_width) / float(m_height);
  XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);

  // 4. 存到 XMFLOAT4X4 結構，再轉給 D3D
  XMFLOAT4X4 fw, fv, fp;
  XMStoreFloat4x4(&fw, world);
  XMStoreFloat4x4(&fv, view);
  XMStoreFloat4x4(&fp, proj);

  // D3DMATRIX 與 XMFLOAT4X4 記憶體佈局相容，可以 reinterpret_cast
  m_dev->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&fw));
  m_dev->SetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(&fv));
  m_dev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(&fp));
}

void CameraController::Update(float deltaTime) {
  // 簡單插值到目標值
  const float speed = 5.0f;
  m_currentYaw += (m_targetYaw - m_currentYaw) * deltaTime * speed;
  m_currentPitch += (m_targetPitch - m_currentPitch) * deltaTime * speed;
  m_currentDist += (m_targetDist - m_currentDist) * deltaTime * speed;
  m_currentAt = XMVectorLerp(m_currentAt, m_targetAt, deltaTime * speed);
}

XMMATRIX CameraController::GetViewMatrix() const {
  XMVECTOR dir = XMVectorSet(
    cosf(m_currentPitch) * sinf(m_currentYaw),
    sinf(m_currentPitch),
    cosf(m_currentPitch) * cosf(m_currentYaw),
    0
  );
  XMVECTOR eye = m_currentAt - XMVectorScale(dir, m_currentDist);
  XMVECTOR up = XMVectorSet(0, 1, 0, 0);
  return XMMatrixLookAtLH(eye, m_currentAt, up);
}

XMMATRIX CameraController::GetProjMatrix(float aspect) const {
  return XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 1000.0f);
}
