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
#include "AssetManager.h"
#include "JsonConfigManager.h"
#include "SceneManager.h"
// #include "UISystem.h" // Removed - using UIManager only
#include "EventManager.h"

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

  // Step 5: 建立並檢查子系統 - 保持分離的TextureManager以優化不同用途
  // UI紋理: 小圖、高頻存取、長生命週期
  uiTextureManager_ = CreateTextureManager(device);
  if (!uiTextureManager_) return E_FAIL;
  
  // Model紋理: 大圖、場景生命週期、需要mipmap
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

  // 初始化現代架構系統
  if (!InitializeModernSystems()) {
    return E_FAIL;
  }

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
  
  // UI系統測試內容移到GameScene中
  
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
    
    // 更新系統 - 優先使用新架構，如果不可用則使用舊系統
    if (sceneManager_) {
      // 新架構：使用 SceneManager 更新場景
      sceneManager_->Update(0.016f);  // 假設60FPS
      
      // 處理事件佇列
      if (eventManager_) {
        eventManager_->ProcessEvents();
      }
    } else {
      // 舊系統：直接更新 camera
      cameraController_->Update(0.016f);
    }

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
      
      // 渲染 - 優先使用新架構
      if (sceneManager_) {
        // 新架構：使用 SceneManager 渲染場景
        sceneManager_->Render();
        
        // 渲染 UI 系統
        // 暫時停用新 UI 系統
        // if (uiSystem_) {
        //   uiSystem_->Render();
        // }
        
        // 也渲染舊的 UIManager (向後相容)
        if (uiManager_) {
          uiManager_->Render(device.Get());
        }
      } else {
        // 舊系統：直接渲染 Scene3D
        if (scene3D_ && cameraController_) {
          float aspect = static_cast<float>(width_) / static_cast<float>(height_);
          scene3D_->Render(device.Get(), 
                          cameraController_->GetViewMatrix(),
                          cameraController_->GetProjMatrix(aspect),
                          uiManager_.get());
        }
      }
      
      // 後處理 (暫時跳過，因為需要輸入紋理)
      // if (fullScreenQuad_) fullScreenQuad_->Render(device.Get(), inputTexture);
    }
    
    d3dContext_->EndScene();
    d3dContext_->Present();
  }
  
  // 清理場景管理器中的所有場景
  if (sceneManager_) {
    // 通知場景管理器程式即將退出
    if (auto* sm = dynamic_cast<SceneManager*>(sceneManager_.get())) {
      sm->PopAllScenes();
    }
  }
  
  return S_OK;
}

// Get 方法實作 (舊系統)
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

// Get 方法實作 (新架構系統)
ISceneManager* EngineContext::GetSceneManager() { return sceneManager_.get(); }
IAssetManager* EngineContext::GetAssetManager() { return assetManager_.get(); }
// IUISystem* EngineContext::GetUISystem() { return uiSystem_.get(); } // Removed
IEventManager* EngineContext::GetEventManager() { return eventManager_.get(); }
IConfigManager* EngineContext::GetConfigManager() { return configManager_.get(); }
IServiceLocator* EngineContext::GetServices() { return serviceLocator_.get(); }

// 現代架構系統初始化
bool EngineContext::InitializeModernSystems() {
    // 1. 創建事件管理器 (最先創建，其他系統可能需要它)
    eventManager_ = CreateEventManager();
    if (!eventManager_) {
        std::cerr << "Failed to create EventManager" << std::endl;
        return false;
    }
    
    // 2. 創建配置管理器
    configManager_ = CreateConfigManager();
    if (!configManager_) {
        std::cerr << "Failed to create ConfigManager" << std::endl;
        return false;
    }
    
    // 載入預設配置
    if (!LoadConfiguration()) {
        std::cerr << "Failed to load configuration" << std::endl;
        return false;
    }
    
    // 3. 創建資產管理器
    assetManager_ = CreateAssetManager();
    if (!assetManager_) {
        std::cerr << "Failed to create AssetManager" << std::endl;
        return false;
    }
    
    // 初始化資產管理器
    ComPtr<IDirect3DDevice9> device;
    HRESULT hr = d3dContext_->GetDevice(&device);
    if (FAILED(hr)) {
        std::cerr << "Failed to get D3D device for AssetManager" << std::endl;
        return false;
    }
    
    if (!assetManager_->Initialize(device.Get())) {
        std::cerr << "Failed to initialize AssetManager" << std::endl;
        return false;
    }
    
    // 設定資產路徑為當前目錄（test資料夾中的檔案直接放在根目錄）
    assetManager_->SetAssetPath(AssetType::Model, "");    // 模型檔案直接在根目錄
    assetManager_->SetAssetPath(AssetType::Texture, "");  // 紋理檔案直接在根目錄
    
    // 4. UI 系統已移除 - 使用 UIManager
    
    // 5. 創建服務定位器並設置所有服務
    CreateServiceLocator();
    
    // 6. 創建場景管理器
    sceneManager_ = CreateSceneManager();
    if (!sceneManager_) {
        std::cerr << "Failed to create SceneManager" << std::endl;
        return false;
    }
    
    if (!sceneManager_->Initialize(serviceLocator_.get())) {
        std::cerr << "Failed to initialize SceneManager" << std::endl;
        return false;
    }
    
    // 設置 SceneManager 到 ServiceLocator
    serviceLocator_->SetSceneManager(sceneManager_.get());
    
    // UISystem 已移除 - 使用 UIManager
    
    // 註冊 SceneManager 為輸入處理器（高優先權，在 UIManager 之後）
    if (auto* sm = dynamic_cast<SceneManager*>(sceneManager_.get())) {
        inputHandler_->RegisterListener(sm);
    }
    
    return true;
}

void EngineContext::CreateServiceLocator() {
    serviceLocator_ = std::make_unique<ServiceLocator>();
    
    // 設置現代架構服務
    serviceLocator_->SetAssetManager(assetManager_.get());
    serviceLocator_->SetConfigManager(configManager_.get());
    serviceLocator_->SetEventManager(eventManager_.get());
    serviceLocator_->SetUIManager(uiManager_.get());
    serviceLocator_->SetCameraController(cameraController_.get());
    
    // 獲取 D3D 設備並設置
    ComPtr<IDirect3DDevice9> device;
    if (SUCCEEDED(d3dContext_->GetDevice(&device))) {
        serviceLocator_->SetDevice(device.Get());
    }
    
    // 設置舊架構服務（為了向後相容）
    serviceLocator_->SetTextureManager(modelTextureManager_.get()); // 使用 model texture manager 作為預設
    serviceLocator_->SetEffectManager(effectManager_.get());
    serviceLocator_->SetD3DContext(d3dContext_.get());
    serviceLocator_->SetModelManager(modelManager_.get());
    serviceLocator_->SetLightManager(lightManager_.get());
    serviceLocator_->SetScene3D(scene3D_.get());
    serviceLocator_->SetInputHandler(inputHandler_.get());
    serviceLocator_->SetPostProcessor(fullScreenQuad_.get());
}

bool EngineContext::LoadConfiguration() {
    // 嘗試載入配置文件，如果不存在則使用預設值
    bool configLoaded = configManager_->LoadConfig("config/engine.json");
    
    if (!configLoaded) {
        
        // 設置預設資產路徑
        configManager_->SetString("assets.models.path", "models/");
        configManager_->SetString("assets.textures.path", "textures/");
        configManager_->SetString("assets.effects.path", "effects/");
        configManager_->SetInt("window.width", width_);
        configManager_->SetInt("window.height", height_);
        configManager_->SetBool("engine.debug_mode", true);
    }
    
    return true;
}
