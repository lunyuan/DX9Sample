// EngineContext.cpp
#include "EngineContext.h"
#include "TextureManager.h"
#include "EffectManager.h"
#include "D3DContext.h"
#include "ModelManager.h"
#include "LightManager.h"
#include "Scene3D.h"
#include "UIManager.h"
#include "InputHandler.h"
#include "CameraController.h"
#include "FullScreenQuad.h"
#include "XModelLoader.h"
#include "DirectionalLight.h"

// Factory 函式
std::unique_ptr<IEngineContext> CreateEngineContext() {
  return std::make_unique<EngineContext>();
}

EngineContext::EngineContext() {}
EngineContext::~EngineContext() {}


// Step 1: 函式簽章確認
STDMETHODIMP EngineContext::Initialize(
  HWND hwnd,
  UINT width,
  UINT height
) {
  // Step 2: 參數檢查
  if (!IsWindow(hwnd)) {
    return E_INVALIDARG;
  }
  if (width == 0 || height == 0) {
    return E_INVALIDARG;
  }

  hwnd_ = hwnd;
  width_ = width;
  height_ = height;

  // Step 3: 建立 D3DContext 並初始化 DirectX
  d3dContext_ = CreateD3DContext();
  
  // 先嘗試硬體HAL
  HRESULT hr = d3dContext_->Init(hwnd, width, height, D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
  
  if (FAILED(hr)) {
    // 硬體HAL失敗，嘗試軟體頂點處理的HAL
    hr = d3dContext_->Init(hwnd, width, height, D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING);
  }
  
  if (FAILED(hr)) {
    // HAL完全失敗，fallback到REF (參考光柵化器)
    hr = d3dContext_->Init(hwnd, width, height, D3DDEVTYPE_REF, D3DCREATE_SOFTWARE_VERTEXPROCESSING);
  }
  
  if (FAILED(hr)) return hr;

  // Step 4: 取得 DirectX 裝置
  ComPtr<IDirect3DDevice9> device;
  hr = d3dContext_->GetDevice(&device);
  if (FAILED(hr)) return hr;

  // Step 5: 建立並檢查子系統 - 創建兩個獨立的TextureManager
  uiTextureManager_ = CreateTextureManager(device);
  if (!uiTextureManager_) return E_FAIL;
  
  modelTextureManager_ = CreateTextureManager(device);
  if (!modelTextureManager_) return E_FAIL;

  effectManager_ = CreateEffectManager();
  if (!effectManager_) return E_FAIL;

  modelManager_ = CreateModelManager(std::make_unique<XModelLoader>(), modelTextureManager_.get());
  if (!modelManager_) return E_FAIL;

  lightManager_ = CreateLightManager();
  if (!lightManager_) return E_FAIL;

  // 添加一個基本的方向光
  auto dirLight = std::make_unique<DirectionalLight>(
    1.0f, 1.0f, 1.0f,  // 白色光
    -0.577f, -0.577f, 0.577f  // 光線方向
  );
  dirLight_ = dirLight.get();
  lightManager_->AddLight(dirLight.release());

  scene3D_ = CreateScene3D();
  if (!scene3D_) return E_FAIL;

  uiManager_ = CreateUIManager(uiTextureManager_.get());
  if (!uiManager_) return E_FAIL;
  
  // 初始化UIManager
  hr = uiManager_->Init(device.Get());
  if (FAILED(hr)) return hr;

  inputHandler_ = CreateInputHandler(hwnd_);
  if (!inputHandler_) return E_FAIL;

  // 先註冊UIManager，讓UI有優先處理權
  inputHandler_->RegisterListener(uiManager_.get());

  cameraController_ = CreateCameraController(device.Get(), width_, height_);
  if (!cameraController_) return E_FAIL;
  inputHandler_->RegisterListener(cameraController_.get());

  fullScreenQuad_ = CreateFullScreenQuad();
  if (!fullScreenQuad_) return E_FAIL;

  return S_OK;
}

STDMETHODIMP EngineContext::LoadAssets(
  const std::wstring& modelFile,
  const std::wstring& textureFile) {
  
  if (!scene3D_ || !lightManager_ || !d3dContext_) return E_FAIL;
  
  // 取得 DirectX 裝置
  ComPtr<IDirect3DDevice9> device;
  HRESULT hr = d3dContext_->GetDevice(&device);
  if (FAILED(hr)) return hr;
  
  // 載入 3D 模型和材質到 Scene3D
  hr = scene3D_->Init(
    device.Get(),
    lightManager_.get(),
    modelFile,
    textureFile
  );
  if (FAILED(hr)) return hr;
  
  // 添加新UI系統測試內容
  if (uiManager_) {
    // 創建背景圖片 (可拖曳)
    auto* bgImage = uiManager_->CreateImage(L"bg.bmp", 50, 50, 200, 150, true);
    
    // 在背景圖片上添加兩個按鈕 (相對座標，跟隨父容器移動)
    auto* button1 = uiManager_->CreateButton(L"按鈕1", 10, 10, 80, 30, 
      []() { 
        OutputDebugStringA("按鈕1被點擊了!\n");
        MessageBoxA(nullptr, "按鈕1被點擊了!", "UI測試", MB_OK);
      }, bgImage,          // parent
      L"bt.bmp");         // normalImage
    
    auto* button2 = uiManager_->CreateButton(L"按鈕2", 10, 50, 80, 30,
      []() { 
        OutputDebugStringA("按鈕2被點擊了!\n");
        MessageBoxA(nullptr, "按鈕2被點擊了!", "UI測試", MB_OK);
      }, bgImage,          // parent
      L"bt.bmp");         // normalImage
    
    // 添加編輯框
    auto* editBox = uiManager_->CreateEdit(10, 90, 100, 25, bgImage);
    
    // 創建文字層 (向後相容)
    int textLayer = uiManager_->CreateLayer(L"Text", 1.0f);
    uiManager_->AddText(L"新UI系統測試", 10, 10, 300, 30, 0xFFFFFF00, textLayer);
    uiManager_->AddText(L"右鍵拖曳背景", 10, 45, 300, 30, 0xFF00FF00, textLayer);
    uiManager_->AddText(L"按鈕支援四狀態", 10, 80, 300, 30, 0xFF00FFFF, textLayer);
  }
  
  return S_OK;
}

// Step 4: 主循環
STDMETHODIMP EngineContext::Run() {
  // Step 5: 訊息迴圈
  MSG msg = {};
  bool running = true;
  while (running) {
    // Step 6: 處理輸入 - 讓InputHandler處理所有訊息
    HRESULT inputResult = inputHandler_->ProcessMessages();
    if (inputResult == S_FALSE) {  // WM_QUIT received
      running = false;
      break;
    }
    
    // 檢查窗口是否還有效
    if (!IsWindow(hwnd_)) {
      running = false;
      break;
    }
    
    cameraController_->Update(0.016f);  // 假設60FPS

    // Step 7: 繪製
    // 清除畫面
    d3dContext_->Clear(D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                      D3DCOLOR_XRGB(64, 128, 255), 1.0f, 0);
    
    d3dContext_->BeginScene();
    
    // 取得 DirectX 裝置
    ComPtr<IDirect3DDevice9> device;
    HRESULT hr = d3dContext_->GetDevice(&device);
    if (SUCCEEDED(hr)) {
      // 應用光照 (Effect 在 Scene3D 中處理)
      if (lightManager_) lightManager_->ApplyAll(device.Get());
      
      // 渲染 3D 場景和 UI (Scene3D 現在負責渲染UI)
      if (scene3D_ && cameraController_) {
        float aspect = static_cast<float>(width_) / static_cast<float>(height_);
        scene3D_->Render(device.Get(), 
                        cameraController_->GetViewMatrix(),
                        cameraController_->GetProjMatrix(aspect),
                        uiManager_.get());
      }
      
      // 後處理 (暫時跳過，因為需要輸入紋理)
      // if (fullScreenQuad_) fullScreenQuad_->Render(device.Get(), inputTexture);
    }
    
    d3dContext_->EndScene();
    d3dContext_->Present();
  }
  return S_OK;
}

// Get 方法實作
ITextureManager* EngineContext::GetTextureManager() { return modelTextureManager_.get(); }
IEffectManager* EngineContext::GetEffectManager() { return effectManager_.get(); }
ID3DContext* EngineContext::GetD3DContext() { return d3dContext_.get(); }
IModelManager* EngineContext::GetModelManager() { return modelManager_.get(); }
ILightManager* EngineContext::GetLightManager() { return lightManager_.get(); }
IScene3D* EngineContext::GetScene3D() { return scene3D_.get(); }
IUIManager* EngineContext::GetUIManager() { return uiManager_.get(); }
IInputHandler* EngineContext::GetInputHandler() { return inputHandler_.get(); }
ICameraController* EngineContext::GetCameraController() { return cameraController_.get(); }
IFullScreenQuad* EngineContext::GetPostProcessor() { return fullScreenQuad_.get(); }
