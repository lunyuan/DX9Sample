#pragma once
#include "IUIManager.h"
#include "ITextureManager.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <wrl/client.h>
#include <d3dx9.h>
using Microsoft::WRL::ComPtr;

struct UILayer {
  bool visible = true;
  float alpha = 1.0f;
  int zOrder = 0;
  std::wstring name;
};

struct UITextElement {
  std::wstring text;
  RECT rect;
  D3DCOLOR color;
  DWORD format;
  int layer;
};

struct UIImageElement {
  std::wstring imagePath;
  RECT destRect;
  D3DCOLOR color;
  bool useTransparency;
  int layer;
  bool draggable = false;
  bool visible = true;
  int id = -1;
};

// 前向宣告
struct UIComponentNew;

// 新UI組件基類
struct UIComponentNew {
  int id;
  int relativeX, relativeY;  // 相對於父組件的座標
  int width, height;
  bool visible = true;
  bool enabled = true;
  
  UIComponentNew* parent = nullptr;
  std::vector<std::unique_ptr<UIComponentNew>> children;
  
  // 計算絕對座標
  virtual RECT GetAbsoluteRect() const {
    int absX = relativeX;
    int absY = relativeY;
    if (parent) {
      RECT parentRect = parent->GetAbsoluteRect();
      absX += parentRect.left;
      absY += parentRect.top;
    }
    return {absX, absY, absX + width, absY + height};
  }
  
  // 事件處理 - 只有當滑鼠在此組件上時才會被調用
  virtual bool OnMouseMove(int x, int y) { return false; }
  virtual bool OnMouseDown(int x, int y, bool isRightButton) { return false; }
  virtual bool OnMouseUp(int x, int y, bool isRightButton) { return false; }
  virtual bool OnKeyDown(WPARAM key) { return false; }
  virtual bool OnChar(WPARAM ch) { return false; }
  
  virtual void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) = 0;
  
  virtual ~UIComponentNew() = default;
};

// 圖片組件
struct UIImageNew : public UIComponentNew {
  std::wstring imagePath;
  D3DCOLOR color = 0xFFFFFFFF;
  bool useTransparency = true;
  bool draggable = false;
  
  void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
  bool OnMouseDown(int x, int y, bool isRightButton) override;
};

// 按鈕組件 - 支援四狀態圖片
struct UIButtonNew : public UIComponentNew {
  std::wstring text;
  
  // 四狀態圖片 (可選)
  std::wstring normalImage;
  std::wstring hoverImage; 
  std::wstring pressedImage;
  std::wstring disabledImage;
  
  // 狀態
  enum class State { Normal, Hover, Pressed, Disabled } state = State::Normal;
  
  // 顏色設定 (當沒有圖片時使用)
  D3DCOLOR textColor = 0xFF000000;
  D3DCOLOR backgroundColor = 0xFFC0C0C0;
  
  std::function<void()> onClick;
  
  void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
  bool OnMouseMove(int x, int y) override;
  bool OnMouseDown(int x, int y, bool isRightButton) override;
  bool OnMouseUp(int x, int y, bool isRightButton) override;
};

// 編輯框組件 - 支援Unicode輸入
struct UIEditNew : public UIComponentNew {
  std::wstring text;
  std::wstring backgroundImage;
  D3DCOLOR textColor = 0xFF000000;
  D3DCOLOR backgroundColor = 0xFFFFFFFF;
  D3DCOLOR borderColor = 0xFF808080;
  
  bool isFocused = false;
  int cursorPos = 0;
  int maxLength = 256;
  
  void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
  bool OnMouseDown(int x, int y, bool isRightButton) override;
  bool OnKeyDown(WPARAM key) override;
  bool OnChar(WPARAM ch) override;
};

// UI容器，支援父子關係
struct UIContainer {
  int id;
  std::wstring backgroundImage;
  
  // 容器絕對位置和大小
  int absoluteX, absoluteY;
  int width, height;
  
  bool draggable = false;
  bool visible = true;
  D3DCOLOR color = 0xFFFFFFFF;
  
  // 子元素
  std::vector<UIButtonNew> childButtons;
  
  // 計算絕對座標
  RECT GetAbsoluteRect() const {
    return {absoluteX, absoluteY, absoluteX + width, absoluteY + height};
  }
  
  // 計算子元素絕對座標
  RECT GetChildAbsoluteRect(const UIButtonNew& child) const {
    return {
      absoluteX + child.relativeX,
      absoluteY + child.relativeY,
      absoluteX + child.relativeX + child.width,
      absoluteY + child.relativeY + child.height
    };
  }
};

// 舊的按鈕結構保持相容性
struct UIButton {
  std::wstring text;
  std::wstring backgroundImage;
  RECT rect;
  D3DCOLOR textColor;
  D3DCOLOR backgroundColor;
  bool useBackgroundImage;
  bool isPressed = false;
  bool isHovered = false;
  bool draggable = false;
  bool visible = true;
  int layer;
  int id;
  std::function<void()> onClick;
};

class UIManager : public IUIManager {
public:
  UIManager(ITextureManager* textureManager = nullptr);
  
  HRESULT Init(IDirect3DDevice9* dev) override;
  HRESULT Render(IDirect3DDevice9* dev) override;

  bool HandleMessage(const MSG& msg) override;
  void RegisterUIListener(IUIInputListener* listener) override {
    uiListeners_.push_back(listener);
  }
  
  // 新增多層UI功能
  int CreateLayer(const std::wstring& name, float alpha = 1.0f) override;
  void SetLayerVisible(int layerId, bool visible) override;
  void SetLayerAlpha(int layerId, float alpha) override;
  
  // 添加UI元素
  void AddText(const std::wstring& text, int x, int y, int width, int height, 
               unsigned long color = 0xFFFFFFFF, int layer = 0) override;
  int AddImage(const std::wstring& imagePath, int x, int y, int width, int height,
                bool useTransparency = true, unsigned long color = 0xFFFFFFFF, int layer = 0, bool draggable = false) override;
  
  // 按鈕功能
  int AddButton(const std::wstring& text, int x, int y, int width, int height,
                std::function<void()> onClick, int layer = 0, bool draggable = false) override;
  int AddImageButton(const std::wstring& imagePath, int x, int y, int width, int height,
                     std::function<void()> onClick, int layer = 0, bool draggable = false) override;
  void SetButtonVisible(int buttonId, bool visible) override;
  void SetImageVisible(int imageId, bool visible) override;
  
  // 鼠標交互
  bool IsPointInTransparentArea(int x, int y, const std::wstring& imagePath, const RECT& rect);
  int GetTopMostElementAt(int x, int y);
  
  void ClearLayer(int layer) override;
  void ClearAll() override;
  
  // 新的組件系統
  UIComponentNew* CreateImage(const std::wstring& imagePath, int x, int y, int width, int height, 
                             bool draggable = false, UIComponentNew* parent = nullptr) override;
  UIComponentNew* CreateButton(const std::wstring& text, int x, int y, int width, int height,
                              std::function<void()> onClick, UIComponentNew* parent = nullptr,
                              const std::wstring& normalImage = L"",
                              const std::wstring& hoverImage = L"", 
                              const std::wstring& pressedImage = L"", 
                              const std::wstring& disabledImage = L"") override;
  UIComponentNew* CreateEdit(int x, int y, int width, int height, UIComponentNew* parent = nullptr,
                            const std::wstring& backgroundImage = L"") override;
  
  // 智能事件處理 - 只傳送事件給相關組件
  UIComponentNew* GetComponentAt(int x, int y);
  void SetFocusedComponent(UIComponentNew* component);

private:
  std::vector<IUIInputListener*> uiListeners_;
  ComPtr<ID3DXFont>   font_;
  ComPtr<ID3DXSprite> sprite_;
  ITextureManager*    textureManager_;
  
  std::vector<UILayer> layers_;
  std::vector<UITextElement> textElements_;
  std::vector<UIImageElement> imageElements_;
  std::vector<UIButton> buttons_;
  
  // 新的容器系統
  std::vector<UIContainer> containers_;
  
  // 新的組件系統
  std::vector<std::unique_ptr<UIComponentNew>> rootComponents_;
  UIComponentNew* focusedComponent_ = nullptr;
  UIComponentNew* hoveredComponent_ = nullptr;
  UIComponentNew* draggedComponent_ = nullptr;
  
  // 交互狀態
  int nextId_ = 0;
  int draggedElementId_ = -1;
  POINT lastMousePos_ = {0, 0};
  bool isDragging_ = false;
  
  void SortElementsByLayer();
  void RenderButtons(IDirect3DDevice9* dev);
  void RenderComponents(IDirect3DDevice9* dev, const std::vector<std::unique_ptr<UIComponentNew>>& components);
};