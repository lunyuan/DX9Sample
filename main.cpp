#define NOMINMAX
#include <windows.h>
#include <streambuf>
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include "Include/IEngineContext.h"
#include "GameScene.h"
#include "PauseScene.h"
#include "SettingsScene.h"
#include "Include/ISceneManager.h"
#include "Include/IEventManager.h"
#include "Src/MultiModelGltfConverter.h"

// 全域 EngineContext 實例
static std::unique_ptr<IEngineContext> g_engine;

// Forward declarations
// 視窗程序
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}
class DebugBuffer : public std::streambuf {
protected:
  virtual int overflow(int c) override {
    if (c != EOF) {
      char ch = static_cast<char>(c);
    }
    return c;
  }
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
  
  // 分配控制台視窗以顯示調試輸出
  AllocConsole();
  freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
  freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
  freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
  std::ios::sync_with_stdio(true);
  

  DebugBuffer debugBuf;
  std::cerr.rdbuf(&debugBuf);

  wchar_t cwd[MAX_PATH];
  GetCurrentDirectoryW(MAX_PATH, cwd);
  wprintf(L"Current Directory: %s\n", cwd);

  // 1. 註冊視窗類別
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = TEXT("DX9Sample");

  if (!RegisterClassEx(&wc)) {
    MessageBox(nullptr, TEXT("RegisterClassEx 失敗"), TEXT("錯誤"), MB_OK);
    return 0;
  }

  // 2. 建立視窗
  const UINT width = 1280;
  const UINT height = 720;
  RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
  HWND hWnd = CreateWindowEx(
    0,
    wc.lpszClassName,
    TEXT("DirectX9 Sample"),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rc.right - rc.left,
    rc.bottom - rc.top,
    nullptr, nullptr,
    hInstance,
    nullptr
  );
  if (!hWnd) {
    MessageBox(nullptr, TEXT("CreateWindowEx 失敗"), TEXT("錯誤"), MB_OK);
    return -1;
  }
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  // 3. 初始化 EngineContext (內部會建立 DirectX 裝置)
  g_engine = CreateEngineContext();
  HRESULT hr = g_engine->Initialize(hWnd, width, height);
  if (FAILED(hr)) {
    MessageBoxW(hWnd, L"EngineContext Initialization Failed", L"Error", MB_OK | MB_ICONERROR);
    return -1;
  }

  // 4. 嘗試載入資源 (舊系統 - 如果失敗也繼續，因為新架構不依賴它)
  hr = g_engine->LoadAssets(
    L"test.x",  // 改用test.x，如果失敗就跳過
    L"test.bmp" // 改用test.bmp
  );
  if (FAILED(hr)) {
    // Warning: Legacy assets loading failed, continuing with new architecture...
    // 不要退出程式，繼續使用新架構
  }


  // 5. 註冊所有場景並設置場景管理
  auto* sceneManager = g_engine->GetSceneManager();
  auto* eventManager = g_engine->GetEventManager();
  if (sceneManager && eventManager) {
    // 註冊所有場景
    sceneManager->RegisterScene("GameScene", CreateGameScene);
    sceneManager->RegisterScene("PauseScene", CreatePauseScene);
    sceneManager->RegisterScene("SettingsScene", CreateSettingsScene);
    
    // 設置場景導航事件處理
    eventManager->Subscribe<PauseMenuAction>([sceneManager](const PauseMenuAction& event) {
      
      if (event.action == "resume") {
        // 彈出暫停場景返回遊戲
        sceneManager->PopScene();
      } else if (event.action == "settings") {
        // 推送設定場景
        sceneManager->PushScene("SettingsScene");
      } else if (event.action == "back_to_pause") {
        // 從設定場景返回暫停場景
        sceneManager->PopScene();
      } else if (event.action == "quit") {
        // 退出應用程式
        PostQuitMessage(0);
      }
    });
    
    // 處理遊戲狀態變更事件（從遊戲場景觸發暫停）
    eventManager->Subscribe<Events::GameStateChanged>([sceneManager](const Events::GameStateChanged& event) {
      
      if (event.previousState == "playing" && event.newState == "paused") {
        // 推送暫停場景
        sceneManager->PushScene("PauseScene");
      }
    });
    
    // 初始切換到遊戲場景
    if (!sceneManager->SwitchToScene("GameScene")) {
      MessageBoxW(hWnd, L"Failed to initialize GameScene", L"Error", MB_OK | MB_ICONERROR);
      return -1;
    }
    
    
  } else {
    // Warning: SceneManager or EventManager not available, using legacy mode
  }

  // 6. 執行主迴圈
  hr = g_engine->Run();
  if (FAILED(hr)) {
    MessageBoxW(hWnd, L"EngineContext Run Failed", L"Error", MB_OK | MB_ICONERROR);
  }

  // 5. 清理並退出
  g_engine.reset();
  UnregisterClassW(wc.lpszClassName, hInstance);
  return 0;
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
 // //light.Direction = { -1.0f, -1.0f, 0.0f };
 // g_pd3dDevice->SetLight(0, &light);
 // g_pd3dDevice->LightEnable(0, TRUE);
 // g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

 // //Loader::LoadGltf("AnimatedModel.glb", mesh, skel);
 // //Exporter::ExportGltf("Converted.glb", mesh, skel);

 // std::vector<DirectX::XMFLOAT4X4> globals;
 // float t = 0.0f;
 // MSG msg;
 // while (true) {
 //   if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
 //     if (msg.message == WM_QUIT) break;
 //     TranslateMessage(&msg);
 //     DispatchMessage(&msg);
 //   }
 //   t += 0.016f;
 //   if (!skel.animations.empty()) {
 //     if (t > skel.animations[0].duration) t = 0;
 //     AnimationPlayer::ComputeGlobalTransforms(skel, skel.animations[0], t, globals);
 //   }
 //   SetupMatrices(1280, 720);
 //   SetupCamera(g_pd3dDevice, 1280, 720);
 //   
 //   // Step 1: 清除畫面 + ZBuffer //
 //   g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
 //     D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
 //   // Step 2: 設定 Render State 啟用 ZBuffer //
 //   g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
 //   g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE); // 關燈光
 //   g_pd3dDevice->SetFVF(Vertex);
 //   g_pd3dDevice->BeginScene();
 //   mesh.Draw(g_pd3dDevice);
 //   if (!globals.empty()) {
 //     Visualizer::DrawJoints(g_pd3dDevice, skel, globals.data());
 //     Visualizer::DrawWeights(g_pd3dDevice, mesh, skel, globals.data());
 //   }
 //   g_pd3dDevice->EndScene();
 //   g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
 // }
 // return 0;
}
