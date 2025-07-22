#pragma once
#include <d3d9.h>
#include <string>
#include <functional>
#include <memory>
#include "IInputListener.h"
#include "IUIInputListener.h"
#include "IUIListener.h"

// 前向宣告新UI組件
struct UIComponentNew;

/// <summary>
/// UI 元件渲染管理，同時也是輸入訊息的分派器
/// </summary>
struct IUIManager : public IInputListener {
  virtual ~IUIManager() {}

  /// 註冊一個自訂的 UI 輸入監聽器 (例如文字編輯框、按鈕等)
  virtual void RegisterUIListener(IUIInputListener* listener) = 0;

  /// 初始化所需的 D3D9 資源（字型、Sprite…）
  virtual HRESULT Init(IDirect3DDevice9* device) = 0;

  /// 每幀呼叫以繪製 UI
  virtual HRESULT Render(IDirect3DDevice9* device) = 0;
  
  // 新增多層UI功能
  virtual int CreateLayer(const std::wstring& name, float priority = 0.0f, float alpha = 1.0f) = 0;
  virtual void SetLayerVisible(int layerId, bool visible) = 0;
  virtual void SetLayerAlpha(int layerId, float alpha) = 0;
  
  // 添加UI元素
  virtual int AddText(const std::wstring& text, int x, int y, int width, int height, 
                      unsigned long color = 0xFFFFFFFF, int layer = 0) = 0;
  virtual void UpdateText(int textId, const std::wstring& newText) = 0;
  virtual int AddImage(const std::wstring& imagePath, int x, int y, int width, int height,
                       bool useTransparency = true, unsigned long color = 0xFFFFFFFF, int layer = 0, bool draggable = false) = 0;
  
  // 按鈕功能
  virtual int AddButton(const std::wstring& text, int x, int y, int width, int height,
                        std::function<void()> onClick, int layer = 0, bool draggable = false) = 0;
  virtual int AddImageButton(const std::wstring& imagePath, int x, int y, int width, int height,
                             std::function<void()> onClick, int layer = 0, bool draggable = false) = 0;
  virtual void SetButtonVisible(int buttonId, bool visible) = 0;
  virtual void SetImageVisible(int imageId, bool visible) = 0;
  
  virtual void ClearLayer(int layer) = 0;
  virtual void ClearAll() = 0;
  
  // 新的組件系統接口
  virtual UIComponentNew* CreateImage(const std::wstring& imagePath, int x, int y, int width, int height, 
                                     bool draggable = false, UIComponentNew* parent = nullptr,
                                     bool allowDragFromTransparent = false) = 0;
  virtual UIComponentNew* CreateButton(const std::wstring& text, int x, int y, int width, int height,
                                      std::function<void()> onClick, UIComponentNew* parent = nullptr,
                                      const std::wstring& normalImage = L"",
                                      const std::wstring& hoverImage = L"", 
                                      const std::wstring& pressedImage = L"", 
                                      const std::wstring& disabledImage = L"") = 0;
  virtual UIComponentNew* CreateEdit(int x, int y, int width, int height, UIComponentNew* parent = nullptr,
                                    const std::wstring& backgroundImage = L"") = 0;
  
  // 按名稱或ID查找組件
  virtual UIComponentNew* FindComponentByName(const std::wstring& name) = 0;
  virtual UIComponentNew* FindComponentById(int id) = 0;
  
  // UI事件監聽器管理
  virtual void AddUIListener(IUIListener* listener) = 0;
  virtual void RemoveUIListener(IUIListener* listener) = 0;
  
  // 序列化支援
  virtual const std::vector<std::unique_ptr<UIComponentNew>>& GetRootComponents() const = 0;
  
  // 添加元件到根層級
  virtual void AddComponent(std::unique_ptr<UIComponentNew> component) = 0;
};

// Forward declaration
struct ITextureManager;

/// <summary>Factory 函式：建立預設實作的 UIManager。</summary>
std::unique_ptr<IUIManager> CreateUIManager(ITextureManager* textureManager = nullptr);