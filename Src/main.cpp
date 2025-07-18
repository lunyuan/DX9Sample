#define NOMINMAX
#include <windows.h>
#define DIRECT3D_VERSION 0x0900
#include <d3d9.h>
#include <DirectXMath.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "SkinMesh.h"
#include "Skeleton.h"
#include "AnimationPlayer.h"
#include "Visualizer.h"
#include "XFileLoader.h"
#include "Loader.h"
#include "Exporter.h"
#include "DirectXMath.h"
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

using namespace DirectX;

static IDirect3D9* g_pD3D = nullptr;
 IDirect3DDevice9* g_pd3dDevice = nullptr;

// 角度與距離
static float g_targetYaw = 0.0f, g_currentYaw = 0.0f;
static float g_targetPitch = 0.0f, g_currentPitch = 0.0f;
static float g_targetDist = 5.0f, g_currentDist = 5.0f;

// 目標位置 & 當前位置 (平移、聚焦用)
static XMVECTOR g_targetAt = XMVectorZero(), g_currentAt = XMVectorZero();

// 滑鼠狀態
static POINT  g_lastMouse = { 0,0 };
static bool   g_orbiting = false;
static bool   g_panning = false;
static bool   g_dollying = false;

// 參數
static constexpr float kSmoothing = 0.1f;
static constexpr float kZoomSpeed = 0.0015f;
static constexpr float kDragZoomSpd = 0.05f;   // 右鍵拖曳縮放速度
static constexpr float kPanSpeed = 0.002f; // 中鍵拖曳平移速度
static constexpr float kMinDist = 0.1f;
static constexpr float kMaxDist = 200.0f;
// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitD3D(HWND hWnd);
void Cleanup();
void Render();
// 每幀或初始化時呼叫
void SetupMatrices(int width, int height);
void SetupCamera(IDirect3DDevice9* dev, int width, int height);

class DebugBuffer : public std::streambuf {
protected:
  virtual int overflow(int c) override {
    if (c != EOF) {
      char ch = static_cast<char>(c);
      OutputDebugStringA(std::string(1, ch).c_str());
    }
    return c;
  }
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {

  DebugBuffer debugBuf;
  std::cerr.rdbuf(&debugBuf);

  wchar_t cwd[MAX_PATH];
  GetCurrentDirectoryW(MAX_PATH, cwd);
  wprintf(L"Current Directory: %s\n", cwd);

  // 1. 註冊視窗類別
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = TEXT("DX9Sample");

  if (!RegisterClassEx(&wc)) {
    MessageBox(nullptr, TEXT("RegisterClassEx 失敗"), TEXT("錯誤"), MB_OK);
    return 0;
  }

  // 2. 建立視窗
  HWND hWnd = CreateWindowEx(
    0,
    wc.lpszClassName,
    TEXT("DirectX9 Sample"),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    1280, 720,
    nullptr, nullptr,
    hInstance, nullptr
  );
  if (!hWnd) {
    MessageBox(nullptr, TEXT("CreateWindowEx 失敗"), TEXT("錯誤"), MB_OK);
    return 0;
  }
  ShowWindow(hWnd, nCmdShow);

  // 3. 初始化 Direct3D
  if (!InitD3D(hWnd)) {
   // Cleanup();
    MessageBox(nullptr, TEXT("InitD3D 失敗"), TEXT("錯誤"), MB_OK);
    return 0;
  }
  g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);       // 關閉固定管線光照
  g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // 關閉背面剔除，方便檢查
  D3DLIGHT9 light = {};
  float intensity = 1.0f;

  light.Type = D3DLIGHT_DIRECTIONAL;
  light.Diffuse.r = intensity;
  light.Diffuse.g = intensity;
  light.Diffuse.b = intensity * 0.8f;
  D3DVECTOR direction;
  direction.x = -0.7f;
  direction.y = -0.7f;
  direction.z = 0.0f;
  light.Direction = direction;
  //light.Direction = { -1.0f, -1.0f, 0.0f };
  g_pd3dDevice->SetLight(0, &light);
  g_pd3dDevice->LightEnable(0, TRUE);
  g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
  // Setup window, call InitD3D...
  SkinMesh mesh; Skeleton skel;
  //XFileLoader::Load(L"world.x", g_pd3dDevice, mesh, skel);
  mesh.CreateBuffers(g_pd3dDevice);
 // mesh.SetTexture(g_pd3dDevice, "World.bmp");

  //Loader::LoadGltf("AnimatedModel.glb", mesh, skel);
  //Exporter::ExportGltf("Converted.glb", mesh, skel);

  std::vector<DirectX::XMFLOAT4X4> globals;
  float t = 0.0f;
  MSG msg;
  while (true) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    t += 0.016f;
    if (!skel.animations.empty()) {
      if (t > skel.animations[0].duration) t = 0;
      AnimationPlayer::ComputeGlobalTransforms(skel, skel.animations[0], t, globals);
    }
    SetupMatrices(1280, 720);
    SetupCamera(g_pd3dDevice, 1280, 720);
    
    g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
      D3DCOLOR_XRGB(30, 30, 30), 1.0f, 0);
    g_pd3dDevice->BeginScene();
    mesh.Draw(g_pd3dDevice);
    if (!globals.empty()) {
      Visualizer::DrawJoints(g_pd3dDevice, skel, globals.data());
      Visualizer::DrawWeights(g_pd3dDevice, mesh, skel, globals.data());
    }
    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
  }
  return 0;
}

// 初始化 D3D9 裝置
bool InitD3D(HWND hWnd) {
  g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
  if (!g_pD3D) return false;

  D3DPRESENT_PARAMETERS pp = {};
  pp.Windowed = TRUE;
  pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp.BackBufferFormat = D3DFMT_X8R8G8B8;
  pp.BackBufferWidth = 1280;
  pp.BackBufferHeight = 720;
  pp.EnableAutoDepthStencil = TRUE;
  pp.AutoDepthStencilFormat = D3DFMT_D24S8;

  HRESULT hr = g_pD3D->CreateDevice(
    D3DADAPTER_DEFAULT,
    D3DDEVTYPE_HAL,
    hWnd,
    D3DCREATE_HARDWARE_VERTEXPROCESSING,
    &pp,
    &g_pd3dDevice
  );
  return SUCCEEDED(hr);
}

// 每幀或初始化時呼叫
void SetupMatrices(int width, int height)
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

  float aspect = float(width) / float(height);
  XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);

  // 4. 存到 XMFLOAT4X4 結構，再轉給 D3D
  XMFLOAT4X4 fw, fv, fp;
  XMStoreFloat4x4(&fw, world);
  XMStoreFloat4x4(&fv, view);
  XMStoreFloat4x4(&fp, proj);

  // D3DMATRIX 與 XMFLOAT4X4 記憶體佈局相容，可以 reinterpret_cast
  g_pd3dDevice->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&fw));
  g_pd3dDevice->SetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(&fv));
  g_pd3dDevice->SetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(&fp));
}

// 繪製一幀
void Render() {
  // 清除畫面與深度
  g_pd3dDevice->Clear(0, nullptr,
    D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
    D3DCOLOR_XRGB(30, 30, 30),
    1.0f, 0);

  if (SUCCEEDED(g_pd3dDevice->BeginScene())) {
    // TODO: 在這裡加入你的繪製程式碼
    g_pd3dDevice->EndScene();
  }

  g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

// 釋放資源
void Cleanup() {
  if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
  if (g_pD3D) { g_pD3D->Release();       g_pD3D = nullptr; }
}

// main loop 或 RenderFrame 前呼叫
void SetupCamera(IDirect3DDevice9* dev, int width, int height)
{
  // 1. 平滑插值 Yaw/Pitch/Dist/At
  g_currentYaw += (g_targetYaw - g_currentYaw) * kSmoothing;
  g_currentPitch += (g_targetPitch - g_currentPitch) * kSmoothing;
  g_currentDist += (g_targetDist - g_currentDist) * kSmoothing;
  g_currentAt = XMVectorLerp(g_currentAt, g_targetAt, kSmoothing);

  // 2. 計算攝影機位置
  XMVECTOR dir = XMVectorSet(
    cosf(g_currentPitch) * sinf(g_currentYaw),
    sinf(g_currentPitch),
    cosf(g_currentPitch) * cosf(g_currentYaw),
    0.0f
  );
  XMVECTOR camPos = g_currentAt - dir * g_currentDist;

  // 3. 建立 View 與 Projection
  XMMATRIX view = XMMatrixLookAtLH(camPos, g_currentAt, XMVectorSet(0, 1, 0, 0));
  XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,
    float(width) / float(height), 0.1f, 100.0f);

  // 4. 存成 D3DMATRIX 並設定
  XMFLOAT4X4 vf, pf;
  XMStoreFloat4x4(&vf, view);
  XMStoreFloat4x4(&pf, proj);
  g_pd3dDevice->SetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(&vf));
  g_pd3dDevice->SetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(&pf));

  // （如有需要也可在此重設 World）
}

// 視窗程序
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_LBUTTONDOWN:
    g_orbiting = true;
    g_lastMouse.x = LOWORD(lParam);
    g_lastMouse.y = HIWORD(lParam);
    SetCapture(hWnd);
    return 0;

  case WM_MBUTTONDOWN:
    g_panning = true;
    g_lastMouse.x = LOWORD(lParam);
    g_lastMouse.y = HIWORD(lParam);
    SetCapture(hWnd);
    return 0;

  case WM_RBUTTONDOWN:
    g_dollying = true;
    g_lastMouse.x = LOWORD(lParam);
    g_lastMouse.y = HIWORD(lParam);
    SetCapture(hWnd);
    return 0;

  case WM_LBUTTONUP:
    g_orbiting = false;
    ReleaseCapture();
    return 0;

  case WM_MBUTTONUP:
    g_panning = false;
    ReleaseCapture();
    return 0;

  case WM_RBUTTONUP:
    g_dollying = false;
    ReleaseCapture();
    return 0;

  case WM_MOUSEMOVE:
  {
    POINT cur{ (LONG)LOWORD(lParam), (LONG)HIWORD(lParam) };
    float dx = float(cur.x - g_lastMouse.x);
    float dy = float(cur.y - g_lastMouse.y);

    if (g_orbiting) {
      // Orbit: 調整目標 Yaw/Pitch
      g_targetYaw += dx * 0.005f;
      g_targetPitch += dy * 0.005f;
      // 限制 Pitch
      const float lim = XM_PIDIV2 - 0.01f;
      g_targetPitch = std::clamp(g_targetPitch, -lim, lim);
    }
    else if (g_panning) {
      // Pan: 沿著攝影機的 Right/Up 平移目標點
      // 先計算方向向量
      XMVECTOR dir = XMVectorSet(
        cosf(g_currentPitch) * sinf(g_currentYaw),
        sinf(g_currentPitch),
        cosf(g_currentPitch) * cosf(g_currentYaw),
        0.0f
      );
      XMVECTOR up = XMVectorSet(0, 1, 0, 0);
      XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
      XMVECTOR realUp = XMVector3Normalize(XMVector3Cross(dir, right));
      // 根據滑鼠位移平移
      g_targetAt += (-right * dx + realUp * dy) * (kPanSpeed * g_currentDist);
    }
    else if (g_dollying) {
      // Dolly: 右鍵上下拖控制縮放
      g_targetDist += -dy * kDragZoomSpd;
      g_targetDist = std::clamp(g_targetDist, kMinDist, kMaxDist);
    }

    g_lastMouse = cur;
  }
  return 0;

  case WM_MOUSEWHEEL:
  {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    g_targetDist -= delta * kZoomSpeed;
    g_targetDist = std::clamp(g_targetDist, kMinDist, kMaxDist);
  }
  return 0;

  case WM_KEYDOWN:
    switch (wParam)
    {
    case 'F': // Focus: 把 g_targetAt 設為世界原點，可改成你欲聚焦物件的位置
      g_targetAt = XMVectorZero();
      // 重置距離、角度
      g_targetDist = 25.0f;
      g_targetYaw = g_currentYaw = 0.0f;
      g_targetPitch = g_currentPitch = 0.0f;
      break;
    case VK_ADD:
      g_targetDist = std::clamp(g_targetDist - 0.5f, kMinDist, kMaxDist);
      break;
    case VK_SUBTRACT:
      g_targetDist = std::clamp(g_targetDist + 0.5f, kMinDist, kMaxDist);
      break;
    }
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}
