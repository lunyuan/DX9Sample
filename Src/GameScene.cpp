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
    
    if (!Scene::OnInitialize()) {
        std::cerr << "GameScene: Scene::OnInitialize failed" << std::endl;
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
    OutputDebugStringA("GameScene::OnInitialize - About to load game assets\n");
    try {
        LoadGameAssets();
    } catch (const std::exception& e) {
        std::cerr << "GameScene: Failed to load assets: " << e.what() << std::endl;
        OutputDebugStringA(("GameScene: Exception loading assets: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
    OutputDebugStringA("GameScene::OnInitialize - Finished loading game assets\n");
    
    // 檢查模型是否真的載入了
    if (!loadedModels_.empty()) {
        OutputDebugStringA(("GameScene::OnInitialize - " + std::to_string(loadedModels_.size()) + " models loaded\n").c_str());
    } else {
        OutputDebugStringA("GameScene::OnInitialize - WARNING: No models loaded after LoadGameAssets!\n");
    }
    
    // 創建 UI
    try {
        CreateGameUI();
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
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
        
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
            D3DXVECTOR3 eye(0.0f, 5.0f, -10.0f);
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
            static int renderCount = 0;
            if (renderCount++ % 60 == 0) { // 每秒輸出一次
                OutputDebugStringA(("GameScene: Rendering " + std::to_string(loadedModels_.size()) + " models...\n").c_str());
            }
            
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
            light.Direction = D3DXVECTOR3(-1.0f, -1.0f, 1.0f);
            device->SetLight(0, &light);
            device->LightEnable(0, TRUE);
            
            // 設置環境光
            device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(64, 64, 64));
            
            // 渲染每個模型
            int modelIndex = 0;
            for (const auto& model : loadedModels_) {
                if (model) {
                    // 設置世界變換矩陣（水平排列模型）
                    D3DXMATRIX worldMatrix;
                    D3DXMatrixTranslation(&worldMatrix, modelIndex * 5.0f - (loadedModels_.size() - 1) * 2.5f, 0.0f, 0.0f);
                    device->SetTransform(D3DTS_WORLD, &worldMatrix);
                    
                    // 直接讓模型自己處理材質和紋理
                    // SkinMesh::Draw 會自動處理所有材質和紋理
                    model->mesh.Draw(device);
                    
                    // 調試輸出
                    static int debugFrameCount = 0;
                    if (debugFrameCount++ % 300 == 0) { // 每5秒輸出一次
                        char msg[256];
                        sprintf_s(msg, "Model %d: %zu materials, %zu vertices\n", 
                                 modelIndex, model->mesh.materials.size(), model->mesh.vertices.size());
                        OutputDebugStringA(msg);
                        
                        for (size_t i = 0; i < model->mesh.materials.size(); ++i) {
                            sprintf_s(msg, "  Material %zu: tex=%p\n", i, model->mesh.materials[i].tex);
                            OutputDebugStringA(msg);
                        }
                    }
                    
                    modelIndex++;
                }
            }
            
        } else {
            static int noModelCount = 0;
            if (noModelCount++ % 60 == 0) { // 每秒輸出一次
                OutputDebugStringA("GameScene: No model loaded, rendering test triangle\n");
            }
            
            // 如果沒有載入模型，渲染測試三角形
            struct Vertex {
                float x, y, z;
                unsigned long color;
            };
            
            Vertex vertices[3] = {
                { 0.0f,  3.0f, 0.0f, 0xFFFF0000 }, // 頂部紅色
                {-3.0f, -3.0f, 0.0f, 0xFF00FF00 }, // 左下綠色
                { 3.0f, -3.0f, 0.0f, 0xFF0000FF }  // 右下藍色
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
        OutputDebugStringA("GameScene::OnExit - services_ is null\n");
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
    static int createCount = 0;
    createCount++;
    OutputDebugStringA(("GameScene::CreateGameUI called - count: " + std::to_string(createCount) + "\n").c_str());
    
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
    
    // 輸出調試信息
    char debugMsg[256];
    sprintf_s(debugMsg, "Creating bg.png at (100,100) size %dx%d\n", bgWidth, bgHeight);
    OutputDebugStringA(debugMsg);
    
    // 在背景圖片上添加子元素 (使用相對座標，會跟隨父容器移動)
    auto* pauseButton = uiManager->CreateButton(L"PAUSE", 20, 40, btWidth, btHeight,
        [this]() { 
            // 暫停按鈕點擊處理
            if (services_ && services_->GetSceneManager()) {
                services_->GetSceneManager()->PushScene("PauseScene");
            }
        }, bgImage,          // parent
        L"bt.bmp");         // button image
    
    sprintf_s(debugMsg, "Creating bt.bmp at (20,40) size %dx%d\n", btWidth, btHeight);
    OutputDebugStringA(debugMsg);
    
    // 在按鈕上再添加一層子物件 (使用 7.png)
    auto* buttonChild = uiManager->CreateImage(L"7.png", 10, 10, sevenWidth, sevenHeight, false, pauseButton);
    
    sprintf_s(debugMsg, "Creating 7.png at (10,10) size %dx%d\n", sevenWidth, sevenHeight);
    OutputDebugStringA(debugMsg);
    
    // 輸出調試信息
    OutputDebugStringA("Created button child with 7.png at relative position (10,10) on the button\n");
    
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
    
    sprintf_s(debugMsg, "Creating b-kuang.png at (400,300) size %dx%d [independent draggable UI]\n", bkuangWidth, bkuangHeight);
    OutputDebugStringA(debugMsg);
    
    // 可以在 b-kuang.png 上添加一些子元素來測試
    auto* testButton = uiManager->CreateButton(L"TEST", 50, 50, 100, 40,
        [this]() { 
            OutputDebugStringA("Test button on b-kuang.png clicked!\n");
        }, bkuangImage,     // parent is b-kuang.png
        L"");              // no button image, use default style
    
    OutputDebugStringA("Created test button on b-kuang.png at relative position (50,50)\n");
    
    // 新增一個獨立的按鈕（不在任何可拖曳UI上），用來測試透明區域點擊穿透
    auto* standaloneButton = uiManager->CreateButton(L"Standalone", 150, 400, 120, 40,
        [this]() { 
            OutputDebugStringA("Standalone button clicked! This proves click-through works.\n");
        }, nullptr,        // no parent - this is a root component
        L"");             // no button image, use default style
    
    OutputDebugStringA("Created standalone button at (150,400) to test click-through transparency\n");
    
    // 示範如何使用名稱查找組件
    // 稍後可以通過名稱找到組件，例如：
    // auto* bgComponent = uiManager->FindComponentByName<UIImageNew>(L"bg.png");
    // auto* testButtonComponent = uiManager->FindComponentByName<UIButtonNew>(L"Button_TEST");
    
    // 調試輸出：顯示所有組件的名稱
    OutputDebugStringA("\n=== Created UI Components ===\n");
    OutputDebugStringA(("bg.png - ID: " + std::to_string(bgImage->id) + ", Name: " + 
                       std::string(bgImage->name.begin(), bgImage->name.end()) + "\n").c_str());
    OutputDebugStringA(("b-kuang.png - ID: " + std::to_string(bkuangImage->id) + ", Name: " + 
                       std::string(bkuangImage->name.begin(), bkuangImage->name.end()) + "\n").c_str());
    if (testButton) {
        OutputDebugStringA(("Test Button - ID: " + std::to_string(testButton->id) + ", Name: " + 
                           std::string(testButton->name.begin(), testButton->name.end()) + "\n").c_str());
    }
    if (standaloneButton) {
        OutputDebugStringA(("Standalone Button - ID: " + std::to_string(standaloneButton->id) + ", Name: " + 
                           std::string(standaloneButton->name.begin(), standaloneButton->name.end()) + "\n").c_str());
    }
    OutputDebugStringA("=============================\n\n");
    
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
    OutputDebugStringA(("GameScene: Received PauseMenuAction: " + event.action + "\n").c_str());
    
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
        OutputDebugStringA("GameScene: AssetManager not available\n");
        return;
    }
    
    OutputDebugStringA("GameScene: Starting to load assets...\n");
    
    // 載入遊戲資產
    try {
        // 載入所有模型
        OutputDebugStringA("GameScene: Loading all models from horse_group.x...\n");
        auto models = assetManager->LoadAllModels("horse_group.x");
        if (!models.empty()) {
            loadedModels_ = models;
            OutputDebugStringA(("GameScene: Successfully loaded " + std::to_string(models.size()) + " models from horse_group.x\n").c_str());
            
            // 輸出每個模型的資訊
            int modelIdx = 0;
            for (const auto& model : models) {
                char debugMsg[256];
                sprintf_s(debugMsg, "  Model %d: %zu vertices, %zu indices, %zu materials\n", 
                         modelIdx, model->mesh.vertices.size(), model->mesh.indices.size(), 
                         model->mesh.materials.size());
                OutputDebugStringA(debugMsg);
                
                // 輸出材質資訊
                for (size_t matIdx = 0; matIdx < model->mesh.materials.size(); ++matIdx) {
                    sprintf_s(debugMsg, "    Material %zu: tex=%p\n", 
                             matIdx, model->mesh.materials[matIdx].tex);
                    OutputDebugStringA(debugMsg);
                }
                modelIdx++;
            }
        } else {
            OutputDebugStringA("GameScene: Failed to load models from horse_group.x - returned empty\n");
        }
        
        // 載入紋理
        OutputDebugStringA("GameScene: Loading test.bmp...\n");
        auto texture = assetManager->LoadTexture("test.bmp");
        if (texture) {
            loadedTexture_ = texture;
            OutputDebugStringA("GameScene: Successfully loaded test.bmp texture\n");
            
            // 將 test.bmp 應用到所有模型
            auto* device = services_->GetDevice();
            if (device) {
                for (auto& model : loadedModels_) {
                    if (model) {
                        model->mesh.SetTexture(device, "test.bmp");
                    }
                }
            }
        } else {
            OutputDebugStringA("GameScene: Failed to load test.bmp texture - returned null\n");
        }
        
        // 發送資產載入事件
        Events::AssetLoaded assetEvent;
        assetEvent.assetPath = "test.x";
        assetEvent.assetType = "model";
        assetEvent.success = (!models.empty());
        assetEvent.errorMessage = models.empty() ? "Failed to load models" : "";
        Emit(assetEvent);
                   
    } catch (const std::exception& e) {
        std::cerr << "GameScene: Failed to load assets: " << e.what() << std::endl;
        OutputDebugStringA(("GameScene: Exception in LoadGameAssets: " + std::string(e.what()) + "\n").c_str());
    } catch (...) {
        std::cerr << "GameScene: Unknown exception in LoadGameAssets" << std::endl;
        OutputDebugStringA("GameScene: Unknown exception in LoadGameAssets\n");
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

// IUIListener 實現
void GameScene::OnButtonClicked(UIButtonNew* button) {
    if (!button) return;
    
    // 輸出調試信息
    OutputDebugStringA(("GameScene::OnButtonClicked - Button clicked: " + 
                       std::string(button->name.begin(), button->name.end()) + 
                       " (ID: " + std::to_string(button->id) + ")\n").c_str());
    
    // 可以根據按鈕名稱或ID執行不同的操作
    if (button->name == L"Button_PAUSE") {
        OutputDebugStringA("  -> This is the PAUSE button\n");
    } else if (button->name == L"Button_TEST") {
        OutputDebugStringA("  -> This is the TEST button on b-kuang.png\n");
    } else if (button->name == L"Button_Standalone") {
        OutputDebugStringA("  -> This is the Standalone button\n");
    }
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
    
    OutputDebugStringA(("GameScene::OnComponentClicked - Component clicked: " + 
                       std::string(component->name.begin(), component->name.end()) + 
                       " (Type: " + typeName + ", ID: " + std::to_string(component->id) + ")\n").c_str());
}

// Factory 函式
std::unique_ptr<IScene> CreateGameScene() {
    return std::make_unique<GameScene>();
}