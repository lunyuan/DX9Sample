#pragma once
#include "IUIManager.h"
#include "ITextureManager.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <d3dx9.h>
using Microsoft::WRL::ComPtr;

struct UILayerLegacy {
  bool visible = true;
  float alpha = 1.0f;
  int zOrder = 0;  // Layer ID
  float priority = 0.0f;  // Rendering priority (higher = rendered on top)
  std::wstring name;
};

struct UITextElement {
  int id;
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
class UIManager;

// 新UI組件基類
struct UIComponentNew {
  int id;
  std::wstring name;  // 組件名稱，用於查找
  int relativeX, relativeY;  // 相對於父組件的座標
  int width, height;
  bool visible = true;
  bool enabled = true;
  
  UIComponentNew* parent = nullptr;
  std::vector<std::unique_ptr<UIComponentNew>> children;
  UIManager* manager = nullptr;  // 指向所屬的UIManager，用於發送事件
  
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
  
  // 拖放事件
  virtual bool IsDraggable() const { return false; }  // 是否可被拖曳
  virtual bool CanReceiveDrop() const { return false; }  // 是否可接收拖放
  virtual void OnDragEnter(UIComponentNew* dragged) {}  // 拖曳物進入
  virtual void OnDragLeave(UIComponentNew* dragged) {}  // 拖曳物離開
  virtual bool OnDrop(UIComponentNew* dragged) { return false; }  // 拖放完成，返回true表示接受
  virtual void OnDragStart() {}  // 開始被拖曳
  virtual void OnDragEnd(bool accepted) {}  // 結束拖曳
  
  // 用於保存原始位置
  int originalX = 0;
  int originalY = 0;
  
  virtual void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) = 0;
  
  virtual ~UIComponentNew() = default;
};

// 圖片組件
struct UIImageNew : public UIComponentNew {
  std::wstring imagePath;
  D3DCOLOR color = 0xFFFFFFFF;
  bool useTransparency = true;
  bool draggable = false;
  bool allowDragFromTransparent = false;  // 是否允許從透明區域拖曳，預設為false
  bool canReceiveDrop = false;  // 是否可接收拖放
  
  void Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) override;
  bool OnMouseDown(int x, int y, bool isRightButton) override;
  
  // 拖放實現
  bool IsDraggable() const override { return draggable; }
  bool CanReceiveDrop() const override { return canReceiveDrop; }
  void OnDragEnter(UIComponentNew* dragged) override;
  void OnDragLeave(UIComponentNew* dragged) override;
  bool OnDrop(UIComponentNew* dragged) override;
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
  int CreateLayer(const std::wstring& name, float priority = 0.0f, float alpha = 1.0f) override;
  void SetLayerVisible(int layerId, bool visible) override;
  void SetLayerAlpha(int layerId, float alpha) override;
  
  // 添加UI元素
  int AddText(const std::wstring& text, int x, int y, int width, int height, 
               unsigned long color = 0xFFFFFFFF, int layer = 0) override;
  void UpdateText(int textId, const std::wstring& newText) override;
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
                             bool draggable = false, UIComponentNew* parent = nullptr,
                             bool allowDragFromTransparent = false) override;
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
  UIComponentNew* GetDraggableComponentAt(int x, int y);  // 特殊處理：忽略透明度檢查
  void SetFocusedComponent(UIComponentNew* component);
  
  // 輔助函數 - 獲取圖片尺寸
  bool GetImageSize(const std::wstring& imagePath, int& width, int& height) const;
  
  // 清除透明度快取（用於調試）
  void ClearAlphaMaskCache() { alphaMaskCache_.clear(); }
  
  // 按名稱或ID查找組件
  UIComponentNew* FindComponentByName(const std::wstring& name) override;
  UIComponentNew* FindComponentById(int id) override;
  
  // 模板方法：按名稱查找特定類型的組件
  template<typename T>
  T* FindComponentByName(const std::wstring& name) {
    return dynamic_cast<T*>(FindComponentByName(name));
  }
  
  // 模板方法：按ID查找特定類型的組件
  template<typename T>
  T* FindComponentById(int id) {
    return dynamic_cast<T*>(FindComponentById(id));
  }
  
  // UI事件監聽器管理
  void AddUIListener(IUIListener* listener) override;
  void RemoveUIListener(IUIListener* listener) override;
  
  // 通知監聽器
  void NotifyButtonClicked(UIButtonNew* button);
  void NotifyComponentClicked(UIComponentNew* component);
  
  // 序列化支援
  const std::vector<std::unique_ptr<UIComponentNew>>& GetRootComponents() const override { return rootComponents_; }
  void AddComponent(std::unique_ptr<UIComponentNew> component) override;
  
  // 獲取當前拖放目標
  UIComponentNew* GetDropTarget() const { return dropTarget_; }

private:
  // Alpha 遮罩快取結構 - 避免每次都鎖定紋理
  struct AlphaMask {
    int width;
    int height;
    std::vector<bool> mask;  // true = 不透明, false = 透明
  };
  mutable std::unordered_map<std::wstring, AlphaMask> alphaMaskCache_;
  
  // 建立 Alpha 遮罩
  void BuildAlphaMask(const std::wstring& imagePath) const;
  
private:
  std::vector<IUIInputListener*> uiListeners_;
  ComPtr<ID3DXFont>   font_;
  ComPtr<ID3DXSprite> sprite_;
  ITextureManager*    textureManager_;
  
  std::vector<UILayerLegacy> layers_;
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
  UIComponentNew* pressedComponent_ = nullptr;  // Track which component is pressed
  
  // 交互狀態
  int nextId_ = 0;
  int draggedElementId_ = -1;
  POINT lastMousePos_ = {0, 0};
  POINT dragOffset_ = {0, 0};  // 拖曳開始時的滑鼠相對位置
  bool isDragging_ = false;
  
  // 拖放狀態
  UIComponentNew* dropTarget_ = nullptr;  // 當前的拖放目標
  bool isInDragDropMode_ = false;  // 是否處於拖放模式
  
  // UI事件監聽器列表
  std::vector<IUIListener*> uiEventListeners_;
  
  void SortElementsByLayer();
  void RenderButtons(IDirect3DDevice9* dev);
  void RenderComponents(IDirect3DDevice9* dev, const std::vector<std::unique_ptr<UIComponentNew>>& components);
};