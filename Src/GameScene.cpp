#include "GameScene.h"
#include <iostream>
#include "PauseScene.h"
// #include "IUISystem.h" // Removed - using UIManager only
#include "IUIManager.h"
#include "UIManager.h"  // 需要具體類別來使用 GetImageSize
#include "IAssetManager.h"
#include "IConfigManager.h"
#include "ISceneManager.h"
#include "ICameraController.h"
#include "ModelData.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <chrono>
#include <cmath>
#include "AnimationPlayer.h"
#include "UISerializer.h"
#include <filesystem>
#include "FbxSaver.h"

GameScene::GameScene() 
    : Scene("GameScene")
    , EventListener(nullptr)  // 暫時傳入 nullptr，將在 OnInitialize 中重新初始化
    , pauseButtonPtr_(nullptr)
    , hudLayerId_(-1)
    , gameLayerId_(-1)
    , pauseButtonId_(-1)
    , scoreTextId_(-1)
    , levelTextId_(-1)
    , expTextId_(-1)
    , playerLevel_(1)
    , playerExperience_(0)
    , score_(0)
    , gameTime_(0.0f)
    , isPaused_(false)
    , playerId_("player_001")
{
    SetTransparent(false);
}

GameScene::~GameScene() {
    // 在析構前清理事件訂閱
    // 注意：此時 eventManager_ 可能已經無效，所以設為 nullptr
    SetEventManager(nullptr);
}

bool GameScene::OnInitialize() {
    
    OutputDebugStringA("GameScene::OnInitialize() 開始\n");
    
    if (!Scene::OnInitialize()) {
        std::cerr << "GameScene: Scene::OnInitialize failed" << std::endl;
        OutputDebugStringA("GameScene: Scene::OnInitialize 失敗\n");
        return false;
    }
    
    // 設置 EventListener
    auto* eventManager = services_->GetEventManager();
    if (eventManager) {
        SetEventManager(eventManager);  // 設置 EventListener 的 eventManager_
        
        // 註冊事件處理器
        try {
            LISTEN_TO_EVENT(Events::UIComponentClicked, OnUIComponentClicked);
            LISTEN_TO_EVENT(PlayerLevelUp, OnPlayerLevelUp);
            LISTEN_TO_EVENT(Events::ConfigurationChanged, OnConfigChanged);
            LISTEN_TO_EVENT(PauseMenuAction, OnPauseMenuAction);
            
        } catch (const std::exception& e) {
            std::cerr << "GameScene: Failed to register event handlers: " << e.what() << std::endl;
            return false;
        }
    } else {
        std::cerr << "GameScene: EventManager not available" << std::endl;
        return false;
    }
    
    // 載入遊戲資產
    try {
        LoadGameAssets();
    } catch (const std::exception& e) {
        std::cerr << "GameScene: Failed to load assets: " << e.what() << std::endl;
        std::cerr << "GameScene: Exception loading assets: " << e.what() << std::endl;
        return false;
    }
    
    // 載入骨骼動畫shader
    auto* device = services_->GetDevice();
    if (device) {
        ID3DXBuffer* errorBuffer = nullptr;
        HRESULT hr = D3DXCreateEffectFromFileA(
            device,
            "shaders/skeletal_animation.fx",
            nullptr, // macros
            nullptr, // include
            D3DXSHADER_DEBUG,
            nullptr, // pool
            &skeletalAnimationEffect_,
            &errorBuffer
        );
        
        if (FAILED(hr)) {
            if (errorBuffer) {
                std::cerr << "Failed to load skeletal animation shader: " << (const char*)errorBuffer->GetBufferPointer() << std::endl;
                OutputDebugStringA("Shader compilation error:\n");
                OutputDebugStringA((const char*)errorBuffer->GetBufferPointer());
                errorBuffer->Release();
            }
            OutputDebugStringA("Failed to create skeletal animation effect\n");
        } else {
            OutputDebugStringA("Successfully loaded skeletal animation shader\n");
        }
        
        // 載入簡單貼圖shader
        errorBuffer = nullptr;
        hr = D3DXCreateEffectFromFileA(
            device,
            "shaders/simple_texture.fx",
            nullptr, // macros
            nullptr, // include
            D3DXSHADER_DEBUG,
            nullptr, // pool
            &simpleTextureEffect_,
            &errorBuffer
        );
        
        if (FAILED(hr)) {
            if (errorBuffer) {
                std::cerr << "Failed to load simple texture shader: " << (const char*)errorBuffer->GetBufferPointer() << std::endl;
                errorBuffer->Release();
            }
            OutputDebugStringA("Failed to create simple texture effect\n");
        } else {
            OutputDebugStringA("Successfully loaded simple texture shader\n");
        }
    }
    
    // 檢查模型是否真的載入了
    if (!loadedModels_.empty()) {
    } else {
        std::cerr << "GameScene: WARNING: No models loaded" << std::endl;
    }
    
    // 創建 UI - 嘗試載入已保存的UI佈局
    try {
        LoadUILayout();  // 這會載入保存的UI或創建預設UI
        CreatePersistentHUD();
    } catch (const std::exception& e) {
        std::cerr << "GameScene: Failed to create UI: " << e.what() << std::endl;
        return false;
    }
    
    // 初始化遊戲狀態
    auto* configManager = services_->GetConfigManager();
    if (configManager) {
        playerLevel_ = configManager->GetInt("game.starting_level", 1);
        playerExperience_ = configManager->GetInt("game.starting_experience", 0);
        score_ = configManager->GetInt("game.starting_score", 0);
    }
    
    // 註冊為UI事件監聽器
    auto* uiManager = services_->GetUIManager();
    if (uiManager) {
        uiManager->AddUIListener(this);
    }
    
    return true;
}

void GameScene::OnUpdate(float deltaTime) {
    Scene::OnUpdate(deltaTime);
    
    if (!isPaused_) {
        UpdateGameLogic(deltaTime);
        
        // 更新動畫時間
        animationTime_ += deltaTime;
    }
}

void GameScene::OnRender() {
    Scene::OnRender();
    
    // 渲染 3D 場景
    auto* device = services_->GetDevice();
    if (device) {
        // 設置基本 3D 渲染狀態
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);  // 關閉背面剔除以查看所有面
        // device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);  // 可選：線框模式
        
        // 確保紋理渲染已啟用
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        
        // 設置世界變換矩陣
        D3DXMATRIX worldMatrix;
        D3DXMatrixIdentity(&worldMatrix);
        device->SetTransform(D3DTS_WORLD, &worldMatrix);
        
        // 使用 CameraController 設置視圖和投影矩陣
        auto* cameraController = services_->GetCameraController();
        if (cameraController) {
            // CameraController 的 SetupCamera 方法會自動設置 view 和 projection 矩陣
            cameraController->SetupCamera();
        } else {
            // Fallback: 如果沒有 camera controller，使用預設矩陣
            D3DXMATRIX viewMatrix, projMatrix;
            D3DXVECTOR3 eye(0.0f, 10.0f, -50.0f);  // 往後移動相機以看到更大的模型
            D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
            D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
            D3DXMatrixLookAtLH(&viewMatrix, &eye, &at, &up);
            
            float aspect = 800.0f / 600.0f; // 預設解析度
            D3DXMatrixPerspectiveFovLH(&projMatrix, D3DX_PI / 4.0f, aspect, 1.0f, 1000.0f);
            
            device->SetTransform(D3DTS_VIEW, &viewMatrix);
            device->SetTransform(D3DTS_PROJECTION, &projMatrix);
        }
        
        // 渲染載入的所有3D模型
        if (!loadedModels_.empty()) {
            
            // 啟用光照
            device->SetRenderState(D3DRS_LIGHTING, TRUE);
            
            // 設置簡單的方向光
            D3DLIGHT9 light;
            ZeroMemory(&light, sizeof(D3DLIGHT9));
            light.Type = D3DLIGHT_DIRECTIONAL;
            light.Diffuse.r = 1.0f;
            light.Diffuse.g = 1.0f;
            light.Diffuse.b = 1.0f;
            light.Diffuse.a = 1.0f;
            light.Direction = D3DXVECTOR3(0.0f, -1.0f, 0.0f);  // 光線從上往下照
            device->SetLight(0, &light);
            device->LightEnable(0, TRUE);
            
            // 設置環境光 (增強以便看清模型)
            device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(192, 192, 192));  // 增加環境光亮度
            
            // 確保貼圖被正確渲染
            device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
            
            // 設置貼圖座標wrap模式
            device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            
            // 關閉 Alpha 混合，確保貼圖不會透明
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            
            // 設置貼圖階段狀態
            device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            
            // 渲染每個模型
            int modelIndex = 0;
            for (const auto& model : loadedModels_) {
                if (model) {
                    // 設置世界變換矩陣（保持模型原始位置）
                    D3DXMATRIX worldMatrix;
                    D3DXMatrixIdentity(&worldMatrix);  // 使用單位矩陣，不改變位置
                    device->SetTransform(D3DTS_WORLD, &worldMatrix);
                    
                    
                    // 使用適合的shader
                    // 如果模型沒有骨骼權重數據，使用簡單shader
                    bool hasBoneWeights = false; // TODO: 檢查模型是否真的有骨骼權重
                    bool useSkeletalAnimation = false; // 暫時禁用骨骼動畫
                    bool useSimpleShader = true; // 使用簡單shader
                    
                    // 使用骨骼動畫shader渲染（如果可用）
                    if (useSkeletalAnimation && skeletalAnimationEffect_ && !model->skeleton.joints.empty()) {
                        
                        // 計算骨骼變換矩陣
                        std::vector<DirectX::XMFLOAT4X4> boneMatrices;
                        
                        // 如果有動畫，使用動畫播放器計算矩陣
                        if (!model->skeleton.animations.empty()) {
                            // 確保動畫時間在範圍內
                            float animDuration = model->skeleton.animations[0].duration;
                            float loopedTime = fmodf(animationTime_, animDuration);
                            
                            AnimationPlayer::ComputeGlobalTransforms(
                                model->skeleton,
                                model->skeleton.animations[0], // 使用第一個動畫
                                loopedTime,
                                boneMatrices
                            );
                        } else {
                            // 沒有動畫，使用綁定姿勢
                            boneMatrices.resize(model->skeleton.joints.size());
                            for (size_t i = 0; i < model->skeleton.joints.size(); ++i) {
                                boneMatrices[i] = model->skeleton.joints[i].bindPoseInverse;
                            }
                        }
                        
                        // 使用骨骼動畫渲染
                        model->mesh.DrawWithAnimation(device, skeletalAnimationEffect_, boneMatrices);
                    } else if (useSimpleShader && simpleTextureEffect_) {
                        // 使用簡單shader
                        static int simpleShaderDebugCount = 0;
                        if (simpleShaderDebugCount++ % 300 == 0) {
                            OutputDebugStringA("Using simple texture shader\n");
                        }
                        model->mesh.DrawWithEffect(device, simpleTextureEffect_);
                    } else {
                        // 沒有骨骼或shader，使用普通渲染
                        static int noAnimDebugCount = 0;
                        if (noAnimDebugCount++ % 300 == 0) {
                            char debugMsg[512];
                            sprintf_s(debugMsg, "Using fixed pipeline: useSkeletalAnimation=%s, effect=%p, joints=%zu\n",
                                      useSkeletalAnimation ? "true" : "false",
                                      skeletalAnimationEffect_,
                                      model->skeleton.joints.size());
                            OutputDebugStringA(debugMsg);
                        }
                        // 直接使用 SkinMesh 的 Draw 函數，它會使用已經設定好的貼圖
                        model->mesh.Draw(device);
                    }
                    
                    
                    modelIndex++;
                }
            }
            
        } else {
            std::cout << "GameScene: No models loaded, rendering test triangle" << std::endl;
            
            // 如果沒有載入模型，渲染測試三角形
            device->SetRenderState(D3DRS_LIGHTING, FALSE);
            device->SetTexture(0, nullptr);
            
            struct Vertex {
                float x, y, z;
                unsigned long color;
            };
            
            Vertex vertices[3] = {
                { 0.0f,  10.0f, 0.0f, 0xFFFF0000 }, // 頂部紅色
                {-10.0f, -10.0f, 0.0f, 0xFF00FF00 }, // 左下綠色
                { 10.0f, -10.0f, 0.0f, 0xFF0000FF }  // 右下藍色
            };
            
            device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
            device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex));
        }
    }
}

void GameScene::OnCleanup() {
    // 在調用基類清理前，先清理事件訂閱
    // 這可以防止在清理過程中事件處理器訪問無效的 services_
    SetEventManager(nullptr);
    
    // 取消註冊UI事件監聽器
    if (services_) {
        auto* uiManager = services_->GetUIManager();
        if (uiManager) {
            uiManager->RemoveUIListener(this);
        }
    }
    
    // 清理 UI 元素引用
    pauseButtonPtr_ = nullptr;
    
    // 清理 3D 模型指標
    loadedModels_.clear();
    loadedTexture_.reset();
    
    // 清理shader
    if (skeletalAnimationEffect_) {
        skeletalAnimationEffect_->Release();
        skeletalAnimationEffect_ = nullptr;
    }
    if (simpleTextureEffect_) {
        simpleTextureEffect_->Release();
        simpleTextureEffect_ = nullptr;
    }
    
    Scene::OnCleanup();
}

void GameScene::OnEnter() {
    Scene::OnEnter();
    
    // 恢復遊戲 UI 的互動 - 使用 UIManager
    
    // 發送場景進入事件
    auto* eventManager = services_->GetEventManager();
    if (eventManager) {
        Events::SceneChanged sceneEvent;
        sceneEvent.previousSceneName = "";
        sceneEvent.newSceneName = "GameScene";
        sceneEvent.isOverlay = false;
        Emit(sceneEvent);
    }
    
}

void GameScene::OnExit() {
    // 檢查 services_ 是否有效
    if (!services_) {
        Scene::OnExit();
        return;
    }
    
    // 清理 UI - 使用 UIManager
    auto* uiManager = services_->GetUIManager();
    if (uiManager) {
        if (gameLayerId_ >= 0) {
            uiManager->ClearLayer(gameLayerId_);
        }
        if (hudLayerId_ >= 0) {
            uiManager->ClearLayer(hudLayerId_);
        }
    }
    
    Scene::OnExit();
}

bool GameScene::OnHandleInput(const MSG& msg) {
    // 調試輸入消息
    if (msg.message == WM_KEYDOWN) {
        
        // 處理空格鍵
        if (msg.wParam == VK_SPACE) {
            // 這裡可以觸發暫停選單
            return true; // 表示已處理
        }
    }
    
    // 讓基類處理其他輸入
    return Scene::OnHandleInput(msg);
}

void GameScene::CreateGameUI() {
    // 調試輸出：確認此函數只被調用一次
    
    // 使用舊的 UIManager，它支援父子關係
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        std::cerr << "GameScene: UIManager not available" << std::endl;
        return;
    }
    
    // 獲取圖片實際尺寸
    int bgWidth = 0, bgHeight = 0;
    int btWidth = 0, btHeight = 0;
    int sevenWidth = 0, sevenHeight = 0;
    
    if (auto* uiMgr = dynamic_cast<UIManager*>(uiManager)) {
        uiMgr->GetImageSize(L"bg.png", bgWidth, bgHeight);   // 修正：使用 bg.png
        uiMgr->GetImageSize(L"bt.bmp", btWidth, btHeight);
        uiMgr->GetImageSize(L"7.png", sevenWidth, sevenHeight);  // 修正：使用 7.png
    }
    
    // 如果無法獲取尺寸，使用預設值
    if (bgWidth == 0) bgWidth = 1024;
    if (bgHeight == 0) bgHeight = 128;  // 根據調試輸出調整為 128
    if (btWidth == 0) btWidth = 256;
    if (btHeight == 0) btHeight = 64;
    if (sevenWidth == 0) sevenWidth = 64;
    if (sevenHeight == 0) sevenHeight = 64;
    
    // 創建背景圖片作為父容器 (可拖曳，位置在 100, 100，允許從透明區域拖曳)
    auto* bgImage = uiManager->CreateImage(L"bg.png", 100, 100, bgWidth, bgHeight, true, nullptr, true);
    
    // 將背景圖片轉換為UIImageNew並設定為可接收拖放
    if (auto* bgImgNew = dynamic_cast<UIImageNew*>(bgImage)) {
        bgImgNew->canReceiveDrop = true;  // 設定為可接收拖放
    }
    
    // 輸出調試信息
    
    // 在背景圖片上添加子元素 (使用相對座標，會跟隨父容器移動)
    auto* pauseButton = uiManager->CreateButton(L"PAUSE", 20, 40, btWidth, btHeight,
        [this]() { 
            // 暫停按鈕點擊處理
            if (services_ && services_->GetSceneManager()) {
                services_->GetSceneManager()->PushScene("PauseScene");
            }
        }, bgImage,          // parent
        L"bt.bmp");         // button image
    
    
    // 在按鈕上再添加一層子物件 (使用 7.png，設定為可拖曳)
    auto* buttonChild = uiManager->CreateImage(L"7.png", 10, 10, sevenWidth, sevenHeight, true, pauseButton);
    
    // 將 7.png 設定為可拖曳
    if (auto* sevenImg = dynamic_cast<UIImageNew*>(buttonChild)) {
        sevenImg->draggable = true;  // 可以被拖曳
    }
    
    
    // 添加文字 (注意：AddText 不支援父子關係，所以文字不會跟隨背景圖片移動)
    // 為了實現父子關係，可能需要手動計算位置或修改 UIManager
    uiManager->AddText(L"測試文字", 110, 160, 250, 25, 0xFFFFFFFF, 
                      uiManager->CreateLayer(L"GameUI", 1.0f));
    
    // 添加座標參考點來確認座標系統
    uiManager->AddText(L"(0,0)", 0, 0, 50, 20, 0xFFFF0000, 
                      uiManager->CreateLayer(L"Debug", 2.0f));
    uiManager->AddText(L"(100,100)", 100, 100, 80, 20, 0xFF00FF00, 
                      uiManager->CreateLayer(L"Debug", 2.0f));
    uiManager->AddText(L"(200,200)", 200, 200, 80, 20, 0xFF0000FF, 
                      uiManager->CreateLayer(L"Debug", 2.0f));
    
    // 保存暫停按鈕的指標以便後續使用
    pauseButtonPtr_ = pauseButton;
    
    // 新增第二個獨立的可拖曳UI - b-kuang.png
    // 獲取 b-kuang.png 的尺寸
    int bkuangWidth = 300;  // 使用指定的尺寸
    int bkuangHeight = 238;
    
    // 創建 b-kuang.png 作為另一個獨立的可拖曳UI (位置在 400, 300，不允許從透明區域拖曳)
    auto* bkuangImage = uiManager->CreateImage(L"b-kuang.png", 400, 300, bkuangWidth, bkuangHeight, true, nullptr, false);
    
    // 設定 b-kuang.png 為可接收拖放
    if (auto* bkuangImgNew = dynamic_cast<UIImageNew*>(bkuangImage)) {
        bkuangImgNew->canReceiveDrop = true;  // 可以接收拖放
    }
    
    
    // 可以在 b-kuang.png 上添加一些子元素來測試
    auto* testButton = uiManager->CreateButton(L"TEST", 50, 50, 100, 40,
        [this]() { 
        }, bkuangImage,     // parent is b-kuang.png
        L"");              // no button image, use default style
    
    // 新增一個獨立的按鈕（不在任何可拖曳UI上），用來測試透明區域點擊穿透
    auto* standaloneButton = uiManager->CreateButton(L"Standalone", 150, 400, 120, 40,
        [this]() { 
        }, nullptr,        // no parent - this is a root component
        L"");             // no button image, use default style
    
    // 示範如何使用名稱查找組件
    // 稍後可以通過名稱找到組件，例如：
    // auto* bgComponent = uiManager->FindComponentByName<UIImageNew>(L"bg.png");
    // auto* testButtonComponent = uiManager->FindComponentByName<UIButtonNew>(L"Button_TEST");
    
    
    // 保存UI佈局到檔案
    SaveUILayout();
}

void GameScene::SaveUILayout() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        std::cerr << "GameScene: UIManager not available for saving" << std::endl;
        return;
    }
    
    // 保存UI佈局到檔案
    std::filesystem::path uiLayoutPath = "ui_layout.json";
    if (UISerializer::SaveToFile(uiManager, uiLayoutPath)) {
    } else {
        std::cerr << "GameScene: Failed to save UI layout" << std::endl;
    }
}

void GameScene::LoadUILayout() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        std::cerr << "GameScene: UIManager not available for loading" << std::endl;
        return;
    }
    
    // 檢查檔案是否存在
    std::filesystem::path uiLayoutPath = "ui_layout.json";
    if (!std::filesystem::exists(uiLayoutPath)) {
        CreateGameUI();
        return;
    }
    
    // 載入UI佈局
    if (UISerializer::LoadFromFile(uiManager, uiLayoutPath)) {
        
        // 重新連接事件處理器
        // 找到PAUSE按鈕並重新連接點擊事件
        if (auto* pauseButton = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIButtonNew>(L"Button_PAUSE")) {
            pauseButton->onClick = [this]() { 
                // 暫停按鈕點擊處理
                if (services_ && services_->GetSceneManager()) {
                    services_->GetSceneManager()->PushScene("PauseScene");
                }
            };
        }
        
        // 找到TEST按鈕並重新連接點擊事件
        if (auto* testButton = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIButtonNew>(L"Button_TEST")) {
            testButton->onClick = [this]() {
            };
        }
        
        // 找到Standalone按鈕並重新連接點擊事件
        if (auto* standaloneButton = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIButtonNew>(L"Button_Standalone")) {
            standaloneButton->onClick = [this]() {
            };
        }
        
        // 重新設定拖放配置
        if (auto* bgImage = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIImageNew>(L"bg.png")) {
            bgImage->canReceiveDrop = true;
        }
        
        if (auto* bkuangImage = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIImageNew>(L"b-kuang.png")) {
            bkuangImage->canReceiveDrop = true;
        }
        
        if (auto* sevenImage = dynamic_cast<UIManager*>(uiManager)->FindComponentByName<UIImageNew>(L"7.png")) {
            sevenImage->draggable = true;
        }
    } else {
        std::cerr << "GameScene: Failed to load UI layout, creating default UI" << std::endl;
        CreateGameUI();
    }
}

void GameScene::CreatePersistentHUD() {
    auto* uiManager = services_->GetUIManager();
    if (!uiManager) {
        return;
    }
    
    // 創建 HUD 層級
    hudLayerId_ = uiManager->CreateLayer(L"GameHUD", 2.0f); // 高優先權層級
    
    // 分數顯示
    scoreTextId_ = uiManager->AddText(L"Score: 0", 20, 20, 150, 20, 0xFFFFFFFF, hudLayerId_);
    
    // 等級顯示  
    levelTextId_ = uiManager->AddText(L"Level: 1", 20, 45, 150, 20, 0xFFFFFFFF, hudLayerId_);
    
    // 經驗值文字
    expTextId_ = uiManager->AddText(L"Experience: 0", 20, 70, 200, 20, 0xFFFFFF00, hudLayerId_);
}

void GameScene::OnUIComponentClicked(const Events::UIComponentClicked& event) {
    
    // UIManager 使用回調函式處理點擊，所以這個函式可能不會被呼叫
    // 保留此函式以相容 UISystem 事件
    if (event.componentId == std::to_string(pauseButtonId_)) {
        // 推送暫停場景而不是直接切換暫停狀態
        
        // 透過 SceneManager 推送 PauseScene
        if (services_) {
            auto* sceneManager = services_->GetSceneManager();
            if (sceneManager) {
                sceneManager->PushScene("PauseScene");
            } else {
                std::cerr << "GameScene: SceneManager not available!" << std::endl;
            }
        }
        
        // 發送遊戲狀態改變事件
        Events::GameStateChanged stateEvent;
        stateEvent.previousState = "playing";
        stateEvent.newState = "paused";
        stateEvent.transitionTime = 0.3f;
        Emit(stateEvent);
    }
}

void GameScene::OnPlayerLevelUp(const PlayerLevelUp& event) {
              
    ShowLevelUpEffect(event.playerId, event.newLevel);
    
    // 更新 HUD 顯示
    auto* uiManager = services_->GetUIManager();
    if (uiManager && levelTextId_ >= 0) {
        std::wstring levelText = L"Level: " + std::to_wstring(event.newLevel);
        uiManager->UpdateText(levelTextId_, levelText);
    }
}

void GameScene::OnConfigChanged(const Events::ConfigurationChanged& event) {
}

void GameScene::OnPauseMenuAction(const PauseMenuAction& event) {
    
    if (event.action == "resume") {
        // 恢復遊戲：彈出暫停場景
        if (services_ && services_->GetSceneManager()) {
            services_->GetSceneManager()->PopScene();
        }
        
    } else if (event.action == "settings") {
        // 切換到設定場景
        if (services_ && services_->GetSceneManager()) {
            services_->GetSceneManager()->PushScene("SettingsScene");
        }
        
    } else if (event.action == "quit") {
        // 退出遊戲
        PostQuitMessage(0);
    }
}

void GameScene::UpdateGameLogic(float deltaTime) {
    gameTime_ += deltaTime;
    
    // 模擬遊戲邏輯 - 每5秒增加分數和經驗值
    static float lastScoreTime = 0.0f;
    if (gameTime_ - lastScoreTime >= 5.0f) {
        TriggerScoreIncrease(100, "time_bonus");
        
        // 增加經驗值
        playerExperience_ += 25;
        if (CheckLevelUp(playerId_, playerExperience_)) {
            // 升級邏輯在 CheckLevelUp 中處理
        }
        
        lastScoreTime = gameTime_;
        
        // 更新 HUD 顯示
        auto* uiManager = services_->GetUIManager();
        if (uiManager) {
            if (scoreTextId_ >= 0) {
                std::wstring scoreText = L"Score: " + std::to_wstring(score_);
                uiManager_->UpdateText(scoreTextId_, scoreText);
            }
            if (expTextId_ >= 0) {
                std::wstring expText = L"Experience: " + std::to_wstring(playerExperience_);
                uiManager_->UpdateText(expTextId_, expText);
            }
        }
    }
}

void GameScene::LoadGameAssets() {
    auto* assetManager = services_->GetAssetManager();
    if (!assetManager) {
        std::cerr << "GameScene: AssetManager not available" << std::endl;
        return;
    }
    
    // 載入遊戲資產
    try {
        // 載入 horse_group.fbx
        // 直接載入 FBX 檔案 - 現在支援紋理載入
        auto models = assetManager->LoadAllModels("horse_group.fbx");
        
        if (!models.empty()) {
            std::cout << "GameScene: Successfully loaded " << models.size() << " models from horse_group.fbx" << std::endl;
            
            // 顯示每個載入的模型資訊
            for (size_t i = 0; i < models.size(); ++i) {
                std::cout << "  FBX Model " << i << ": " 
                         << models[i]->mesh.vertices.size() << " vertices, "
                         << models[i]->mesh.indices.size() / 3 << " triangles" << std::endl;
            }
            
            loadedModels_ = models;
        } else {
            std::cerr << "GameScene: Failed to load horse_group.fbx" << std::endl;
        }
        if (!loadedModels_.empty()) {
            
            // 顯示實際載入的模型數量
            char debugMsg[256];
            sprintf_s(debugMsg, "GameScene: 成功載入 %zu 個模型\n", loadedModels_.size());
            OutputDebugStringA(debugMsg);
            
            // 如果只有一個模型，可以選擇是否複製
            // 註解掉複製邏輯，先看原始檔案有多少模型
            /*
            if (loadedModels_.size() == 1) {
                auto originalModel = loadedModels_[0];
                
                // 複製6匹馬（加上原本的共7匹）
                for (int i = 0; i < 6; ++i) {
                    auto copiedModel = std::make_shared<ModelData>(*originalModel);
                    loadedModels_.push_back(copiedModel);
                }
                
                std::cout << "GameScene: Created " << loadedModels_.size() << " horses from single model" << std::endl;
            }
            */
            
            // Save models in different formats for testing
            // SaveModelsInDifferentFormats(models);  // TODO: Implement when savers are ready
            
            for (const auto& model : loadedModels_) {
                if (model) {
                    std::cout << "  - Model with " << model->mesh.vertices.size() << " vertices, " 
                              << model->mesh.indices.size()/3 << " triangles" << std::endl;
                }
            }
        } else {
            std::cerr << "GameScene: Failed to load models from test.x" << std::endl;
        }
        
        // 檢查 FBX 檔案載入後的貼圖狀態
        std::cout << "GameScene: Checking textures loaded from FBX file..." << std::endl;
        
        int modelIndex = 0;
        for (auto& model : loadedModels_) {
            if (model) {
                std::cout << "\nModel " << modelIndex++ << ":" << std::endl;
                std::cout << "  Materials count: " << model->mesh.materials.size() << std::endl;
                std::cout << "  useOriginalTextures: " << (model->useOriginalTextures ? "true" : "false") << std::endl;
                
                // 檢查每個材質的貼圖
                for (size_t i = 0; i < model->mesh.materials.size(); ++i) {
                    const auto& mat = model->mesh.materials[i];
                    std::cout << "  Material " << i << ":" << std::endl;
                    std::cout << "    Texture pointer: " << mat.tex << std::endl;
                    std::cout << "    Diffuse color: (" 
                              << mat.mat.Diffuse.r << ", " 
                              << mat.mat.Diffuse.g << ", " 
                              << mat.mat.Diffuse.b << ", " 
                              << mat.mat.Diffuse.a << ")" << std::endl;
                }
                
                // 檢查 mesh.texture (全域貼圖)
                std::cout << "  Global texture (mesh.texture): " << model->mesh.texture << std::endl;
            }
        }
        
        
        // 發送資產載入事件
        Events::AssetLoaded assetEvent;
        assetEvent.assetPath = "horse_group.fbx";
        assetEvent.assetType = "model";
        assetEvent.success = (!loadedModels_.empty());
        assetEvent.errorMessage = loadedModels_.empty() ? "Failed to load models" : "";
        Emit(assetEvent);
                   
    } catch (const std::exception& e) {
        std::cerr << "GameScene: Failed to load assets: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "GameScene: Unknown exception in LoadGameAssets" << std::endl;
    }
}

void GameScene::ShowLevelUpEffect(const std::string& playerId, int newLevel) {
}

bool GameScene::CheckLevelUp(const std::string& playerId, int experience) {
    int expRequiredForNextLevel = playerLevel_ * 100;
    
    if (experience >= expRequiredForNextLevel) {
        int oldLevel = playerLevel_;
        playerLevel_++;
        
        // 發送升級事件
        PlayerLevelUp levelUpEvent;
        levelUpEvent.playerId = playerId;
        levelUpEvent.oldLevel = oldLevel;
        levelUpEvent.newLevel = playerLevel_;
        levelUpEvent.experienceGained = experience - (oldLevel * 100);
        levelUpEvent.timestamp = gameTime_;
        
        Emit(levelUpEvent);
        
        return true;
    }
    
    return false;
}

void GameScene::TriggerScoreIncrease(int points, const std::string& reason) {
    int oldScore = score_;
    score_ += points;
    
    // 發送分數變更事件
    PlayerScoreChanged scoreEvent;
    scoreEvent.playerId = playerId_;
    scoreEvent.oldScore = oldScore;
    scoreEvent.newScore = score_;
    scoreEvent.scoreDelta = points;
    scoreEvent.reason = reason;
    
    Emit(scoreEvent);
}

void GameScene::SaveModelsInDifferentFormats(const std::vector<std::shared_ptr<ModelData>>& models) {
    // TODO: Implement when model savers are ready
    /*
    // Save each model in FBX and glTF formats
    std::cout << "GameScene: Saving loaded models in different formats..." << std::endl;
    
    // Create savers
    auto fbxSaver = CreateFbxSaver();
    auto gltfSaver = CreateGltfSaver();
    
    if (!fbxSaver || !gltfSaver) {
        std::cerr << "GameScene: Failed to create model savers" << std::endl;
        return;
    }
    
    // Save each model
    int modelIndex = 0;
    for (const auto& model : models) {
        if (!model) continue;
        
        std::string baseName = "test_object_" + std::to_string(modelIndex);
        
        // Convert ModelData to ModelDataV2 for saving
        auto modelV2 = std::make_unique<ModelDataV2>();
        modelV2->name = baseName;
        
        // Copy mesh data
        modelV2->meshes.push_back(MeshDataV2());
        auto& meshV2 = modelV2->meshes.back();
        meshV2.vertices = model->mesh.vertices;
        meshV2.indices = model->mesh.indices;
        
        // Copy skeleton if present
        if (!model->skeleton.joints.empty()) {
            modelV2->skeleton = std::make_unique<SkeletonV2>();
            modelV2->skeleton->joints.reserve(model->skeleton.joints.size());
            for (const auto& joint : model->skeleton.joints) {
                JointV2 jointV2;
                jointV2.name = joint.name;
                jointV2.parentIndex = joint.parentIndex;
                jointV2.bindPoseInverse = joint.bindPoseInverse;
                modelV2->skeleton->joints.push_back(jointV2);
            }
        }
        
        // Copy animations if present
        if (!model->skeleton.animations.empty()) {
            modelV2->animations.reserve(model->skeleton.animations.size());
            for (const auto& anim : model->skeleton.animations) {
                AnimationV2 animV2;
                animV2.name = anim.name;
                animV2.duration = anim.duration;
                animV2.ticksPerSecond = anim.ticksPerSecond;
                // Copy animation channels - simplified for now
                modelV2->animations.push_back(animV2);
            }
        }
        
        // Save as FBX
        try {
            std::string fbxPath = baseName + ".fbx";
            SaveOptions options;
            options.embedTextures = false;
            options.exportAnimations = true;
            
            auto result = fbxSaver->SaveModel(*modelV2, fbxPath, options);
            if (result.success) {
                std::cout << "  Saved: " << fbxPath << std::endl;
            } else {
                std::cerr << "  Failed to save " << fbxPath << ": " << result.errorMessage << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  Exception saving FBX: " << e.what() << std::endl;
        }
        
        // Save as glTF
        try {
            std::string gltfPath = baseName + ".gltf";
            SaveOptions options;
            options.embedTextures = false;
            options.exportAnimations = true;
            options.binary = false;  // Use .gltf instead of .glb
            
            auto result = gltfSaver->SaveModel(*modelV2, gltfPath, options);
            if (result.success) {
                std::cout << "  Saved: " << gltfPath << std::endl;
            } else {
                std::cerr << "  Failed to save " << gltfPath << ": " << result.errorMessage << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  Exception saving glTF: " << e.what() << std::endl;
        }
        
        modelIndex++;
    }
    
    // Test loading the saved files
    std::cout << "\nGameScene: Testing loading of saved files..." << std::endl;
    
    // Test loading FBX
    auto* assetManager = services_->GetAssetManager();
    if (assetManager) {
        try {
            auto fbxModels = assetManager->LoadAllModels("test_object_0.fbx");
            if (!fbxModels.empty()) {
                std::cout << "  Successfully loaded test_object_0.fbx: " << fbxModels.size() << " models" << std::endl;
            } else {
                std::cerr << "  Failed to load test_object_0.fbx" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  Exception loading FBX: " << e.what() << std::endl;
        }
        
        // Test loading glTF
        try {
            auto gltfModels = assetManager->LoadAllModels("test_object_0.gltf");
            if (!gltfModels.empty()) {
                std::cout << "  Successfully loaded test_object_0.gltf: " << gltfModels.size() << " models" << std::endl;
            } else {
                std::cerr << "  Failed to load test_object_0.gltf" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "  Exception loading glTF: " << e.what() << std::endl;
        }
    }
    */
}

// IUIListener 實現
void GameScene::OnButtonClicked(UIButtonNew* button) {
    if (!button) return;
    
    // 輸出調試信息
    // 可以根據按鈕名稱或ID執行不同的操作
}

void GameScene::OnComponentClicked(UIComponentNew* component) {
    if (!component) return;
    
    // 輸出調試信息
    std::string typeName = "Unknown";
    if (dynamic_cast<UIButtonNew*>(component)) {
        typeName = "Button";
    } else if (dynamic_cast<UIImageNew*>(component)) {
        typeName = "Image";
    } else if (dynamic_cast<UIEditNew*>(component)) {
        typeName = "Edit";
    }
}

// Factory 函式
std::unique_ptr<IScene> CreateGameScene() {
    return std::make_unique<GameScene>();
}