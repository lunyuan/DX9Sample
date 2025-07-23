#pragma once

#include <string>
#include <memory>
#include <d3d9.h>

// Forward declarations
struct IAssetManager;
struct IUIManager;
struct IEventManager;
struct IConfigManager;
struct ISceneManager;
struct ICameraController;
struct ITextureManager;
struct IEffectManager;
struct ID3DContext;
struct IModelManager;
struct ILightManager;
struct IScene3D;
struct IInputHandler;
struct IFullScreenQuad;

// 場景狀態
enum class SceneState {
    Uninitialized,
    Initializing,
    Running,
    Paused,
    Transitioning,
    Cleanup
};

// 服務定位器 - 提供場景需要的所有服務
struct IServiceLocator {
    virtual ~IServiceLocator() = default;
    
    // 現代架構服務
    virtual IAssetManager* GetAssetManager() const = 0;
    virtual IUIManager* GetUIManager() const = 0;
    virtual IEventManager* GetEventManager() const = 0;
    virtual IConfigManager* GetConfigManager() const = 0;
    virtual ISceneManager* GetSceneManager() const = 0;
    virtual IDirect3DDevice9* GetDevice() const = 0;
    virtual ICameraController* GetCameraController() const = 0;
    
    // 舊架構服務 (為了向後相容)
    virtual ITextureManager* GetTextureManager() const = 0;
    virtual IEffectManager* GetEffectManager() const = 0;
    virtual ID3DContext* GetD3DContext() const = 0;
    virtual IModelManager* GetModelManager() const = 0;
    virtual ILightManager* GetLightManager() const = 0;
    virtual IScene3D* GetScene3D() const = 0;
    virtual IInputHandler* GetInputHandler() const = 0;
    virtual IFullScreenQuad* GetPostProcessor() const = 0;
    
    // 檢查服務是否有效（未在關閉狀態）
    virtual bool IsValid() const = 0;
};

// 場景介面
struct IScene {
    virtual ~IScene() = default;
    
    // 場景生命週期
    virtual bool Initialize(IServiceLocator* services) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Cleanup() = 0;
    
    // 場景狀態轉換
    virtual void OnEnter() = 0;      // 場景開始時
    virtual void OnExit() = 0;       // 場景結束時
    virtual void OnPause() = 0;      // 場景暫停時 (被其他場景覆蓋)
    virtual void OnResume() = 0;     // 場景恢復時
    
    // 場景查詢
    virtual const std::string& GetName() const = 0;
    virtual SceneState GetState() const = 0;
    virtual bool IsTransparent() const = 0;  // 是否允許背後的場景繼續渲染
    
    // 事件處理
    virtual bool HandleInput(const MSG& msg) = 0;
    
protected:
    // 內部狀態管理
    virtual void SetState(SceneState state) = 0;
};