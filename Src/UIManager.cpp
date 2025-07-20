#include "UIManager.h"
#include <algorithm>
#include <string>

// Factory 函式實作
std::unique_ptr<IUIManager> CreateUIManager(ITextureManager* textureManager) {
  return std::make_unique<UIManager>(textureManager);
}

UIManager::UIManager(ITextureManager* textureManager) 
  : textureManager_(textureManager) {
  // 創建預設層 (layer 0)
  CreateLayer(L"Default", 1.0f);
}

STDMETHODIMP UIManager::Init(IDirect3DDevice9* dev) {
  if (!dev) return E_INVALIDARG;
  HRESULT hr = D3DXCreateFont(dev, 24, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
    L"Arial", &font_);
  if (FAILED(hr)) return hr;
  hr = D3DXCreateSprite(dev, &sprite_);
  return hr;
}

STDMETHODIMP UIManager::Render(IDirect3DDevice9* dev) {
  if (!dev || !font_ || !sprite_) return E_POINTER;
  
  SortElementsByLayer();
  
  // 保存原始渲染狀態
  DWORD oldAlphaBlend, oldSrcBlend, oldDestBlend;
  dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
  dev->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
  dev->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
  
  // 設定alpha混合
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  
  // 設定alpha混合狀態
  sprite_->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
  
  // 渲染圖片元素
  for (const auto& img : imageElements_) {
    if (!img.visible || img.layer >= layers_.size() || !layers_[img.layer].visible) continue;
    
    if (textureManager_) {
      auto texture = textureManager_->Load(img.imagePath);
      if (texture) {
        D3DXVECTOR3 pos(float(img.destRect.left), float(img.destRect.top), 0.0f);
        
        // 計算縮放
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
          float scaleX = float(img.destRect.right - img.destRect.left) / float(desc.Width);
          float scaleY = float(img.destRect.bottom - img.destRect.top) / float(desc.Height);
          
          D3DXMATRIX scale, transform;
          D3DXMatrixScaling(&scale, scaleX, scaleY, 1.0f);
          sprite_->SetTransform(&scale);
        }
        
        // 設定透明度 - 結合圖片顏色和層級透明度
        D3DCOLOR finalColor = img.color;
        if (img.layer < layers_.size()) {
          // 取得原始alpha值
          BYTE originalAlpha = (img.color >> 24) & 0xFF;
          // 應用層級透明度
          BYTE layerAlpha = BYTE(255 * layers_[img.layer].alpha);
          // 結合兩個alpha值
          BYTE combinedAlpha = BYTE((originalAlpha * layerAlpha) / 255);
          finalColor = (img.color & 0x00FFFFFF) | (combinedAlpha << 24);
        }
        
        sprite_->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
                     nullptr, nullptr, &pos, finalColor);
        
        // 重設變換
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        sprite_->SetTransform(&identity);
      }
    }
  }
  
  // 渲染按鈕
  RenderButtons(dev);
  
  // 渲染新組件系統
  RenderComponents(dev, rootComponents_);
  
  // 渲染文字元素
  for (const auto& text : textElements_) {
    if (text.layer >= layers_.size() || !layers_[text.layer].visible) continue;
    
    D3DCOLOR finalColor = text.color;
    if (text.layer < layers_.size()) {
      BYTE alpha = BYTE(255 * layers_[text.layer].alpha);
      finalColor = (finalColor & 0x00FFFFFF) | (alpha << 24);
    }
    
    RECT rect = text.rect;
    font_->DrawText(sprite_.Get(), text.text.c_str(), -1, &rect, text.format, finalColor);
  }
  
  sprite_->End();
  
  // 恢復原始渲染狀態
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
  dev->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
  dev->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
  
  return S_OK;
}


bool UIManager::HandleMessage(const MSG& msg) {
  // 先讓所有註冊的 UI 控件消化
  for (auto l : uiListeners_) {
    if (l->OnUIMessage(msg)) {
      return true;  // 攔截此訊息
    }
  }
  
  // 新的事件委派系統
  int mouseX = LOWORD(msg.lParam);
  int mouseY = HIWORD(msg.lParam);
  
  switch (msg.message) {
    case WM_MOUSEMOVE: {
      UIComponentNew* component = GetComponentAt(mouseX, mouseY);
      
      // 處理hover狀態變化
      if (hoveredComponent_ != component) {
        // 清除舊的hover狀態
        if (hoveredComponent_) {
          if (auto* btn = dynamic_cast<UIButtonNew*>(hoveredComponent_)) {
            if (btn->state == UIButtonNew::State::Hover) {
              btn->state = UIButtonNew::State::Normal;
            }
          }
        }
        
        hoveredComponent_ = component;
        
        // 設定新的hover狀態
        if (hoveredComponent_) {
          hoveredComponent_->OnMouseMove(mouseX, mouseY);
        }
      }
      
      // 處理拖曳
      if (draggedComponent_) {
        int deltaX = mouseX - lastMousePos_.x;
        int deltaY = mouseY - lastMousePos_.y;
        
        // 只有當有實際移動時才更新位置
        if (deltaX != 0 || deltaY != 0) {
          // 調試輸出
          OutputDebugStringA(("Drag: deltaX=" + std::to_string(deltaX) + ", deltaY=" + std::to_string(deltaY) + 
                             ", oldPos=(" + std::to_string(draggedComponent_->relativeX) + "," + std::to_string(draggedComponent_->relativeY) + ")").c_str());
          
          // 移動被拖曳的組件 - 使用1:1移動比率
          draggedComponent_->relativeX += deltaX;
          draggedComponent_->relativeY += deltaY;
          
          // 調試輸出
          OutputDebugStringA((" -> newPos=(" + std::to_string(draggedComponent_->relativeX) + "," + std::to_string(draggedComponent_->relativeY) + ")\n").c_str());
          
          // 更新最後滑鼠位置
          lastMousePos_ = {mouseX, mouseY};
        }
        return true;
      }
      
      return component != nullptr; // 如果滑鼠在UI上，阻止相機處理
    }
    
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      UIComponentNew* component = GetComponentAt(mouseX, mouseY);
      OutputDebugStringA(("Click at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + 
                         "), found component: " + (component ? "YES" : "NO") + "\n").c_str());
      if (component) {
        bool isRightButton = (msg.message == WM_RBUTTONDOWN);
        bool handled = component->OnMouseDown(mouseX, mouseY, isRightButton);
        
        // 設定焦點
        SetFocusedComponent(component);
        
        // 檢查是否開始拖曳
        if (isRightButton) {
          // 只有點擊到可拖曳的圖片組件本身才能拖曳，不允許透過子組件拖曳
          if (auto* img = dynamic_cast<UIImageNew*>(component)) {
            if (img->draggable && !img->parent) { // 只有根組件且可拖曳
              // 檢查點擊位置是否在非透明區域
              RECT componentRect = component->GetAbsoluteRect();
              if (!IsPointInTransparentArea(mouseX, mouseY, img->imagePath, componentRect)) {
                draggedComponent_ = component;
                lastMousePos_ = {mouseX, mouseY};
                SetCapture(msg.hwnd); // 捕獲滑鼠，確保拖曳期間UI有完全控制權
                OutputDebugStringA(("Start dragging background at (" + 
                                  std::to_string(component->relativeX) + "," + 
                                  std::to_string(component->relativeY) + ")\n").c_str());
              }
            }
          }
        }
        
        return handled;
      } else {
        // 點擊空白區域，清除焦點
        SetFocusedComponent(nullptr);
      }
      break;
    }
    
    case WM_LBUTTONUP:
    case WM_RBUTTONUP: {
      if (draggedComponent_) {
        OutputDebugStringA(("End dragging at (" + 
                           std::to_string(draggedComponent_->relativeX) + "," + 
                           std::to_string(draggedComponent_->relativeY) + ")\n").c_str());
        draggedComponent_ = nullptr;
        // 確保釋放滑鼠捕獲，防止相機繼續響應
        ReleaseCapture();
        return true; // 完全攔截事件，不讓相機處理
      }
      
      UIComponentNew* component = GetComponentAt(mouseX, mouseY);
      if (component) {
        bool isRightButton = (msg.message == WM_RBUTTONUP);
        bool handled = component->OnMouseUp(mouseX, mouseY, isRightButton);
        if (handled) {
          ReleaseCapture(); // 確保釋放滑鼠捕獲
        }
        return handled;
      }
      break;
    }
    
    case WM_KEYDOWN: {
      if (focusedComponent_) {
        return focusedComponent_->OnKeyDown(msg.wParam);
      }
      break;
    }
    
    case WM_CHAR: {
      if (focusedComponent_) {
        return focusedComponent_->OnChar(msg.wParam);
      }
      break;
    }
  }
  
  // 舊的事件處理系統已完全移除，只使用新的組件系統
  return false; // 未處理，交給下層或 DefWindowProc
}

int UIManager::CreateLayer(const std::wstring& name, float alpha) {
  UILayer layer;
  layer.name = name;
  layer.alpha = alpha;
  layer.zOrder = static_cast<int>(layers_.size());
  layers_.push_back(layer);
  return static_cast<int>(layers_.size() - 1);
}

void UIManager::SetLayerVisible(int layerId, bool visible) {
  if (layerId >= 0 && layerId < layers_.size()) {
    layers_[layerId].visible = visible;
  }
}

void UIManager::SetLayerAlpha(int layerId, float alpha) {
  if (layerId >= 0 && layerId < layers_.size()) {
    layers_[layerId].alpha = std::clamp(alpha, 0.0f, 1.0f);
  }
}

void UIManager::AddText(const std::wstring& text, int x, int y, int width, int height, 
                       unsigned long color, int layer) {
  UITextElement element;
  element.text = text;
  element.rect = {x, y, x + width, y + height};
  element.color = color;
  element.format = DT_LEFT | DT_TOP;
  element.layer = layer;
  textElements_.push_back(element);
}

int UIManager::AddImage(const std::wstring& imagePath, int x, int y, int width, int height,
                        bool useTransparency, unsigned long color, int layer, bool draggable) {
  UIImageElement element;
  element.imagePath = imagePath;
  element.destRect = {x, y, x + width, y + height};
  element.color = color;
  element.useTransparency = useTransparency;
  element.layer = layer;
  element.id = nextId_++;
  element.visible = true;
  element.draggable = draggable;
  imageElements_.push_back(element);
  return element.id;
}

void UIManager::ClearLayer(int layer) {
  textElements_.erase(
    std::remove_if(textElements_.begin(), textElements_.end(),
      [layer](const UITextElement& e) { return e.layer == layer; }),
    textElements_.end());
    
  imageElements_.erase(
    std::remove_if(imageElements_.begin(), imageElements_.end(),
      [layer](const UIImageElement& e) { return e.layer == layer; }),
    imageElements_.end());
}

void UIManager::ClearAll() {
  textElements_.clear();
  imageElements_.clear();
  buttons_.clear();
}

// 按鈕功能實現
int UIManager::AddButton(const std::wstring& text, int x, int y, int width, int height,
                         std::function<void()> onClick, int layer, bool draggable) {
  UIButton button;
  button.text = text;
  button.rect = {x, y, x + width, y + height};
  button.textColor = 0xFF000000;  // 黑色文字
  button.backgroundColor = 0xFFC0C0C0;  // 灰色背景
  button.useBackgroundImage = false;
  button.onClick = onClick;
  button.layer = layer;
  button.id = nextId_++;
  button.draggable = draggable;
  button.visible = true;
  buttons_.push_back(button);
  return button.id;
}

int UIManager::AddImageButton(const std::wstring& imagePath, int x, int y, int width, int height,
                              std::function<void()> onClick, int layer, bool draggable) {
  UIButton button;
  button.backgroundImage = imagePath;
  button.rect = {x, y, x + width, y + height};
  button.textColor = 0xFFFFFFFF;
  button.backgroundColor = 0xFFFFFFFF;
  button.useBackgroundImage = true;
  button.onClick = onClick;
  button.layer = layer;
  button.id = nextId_++;
  button.draggable = draggable;
  button.visible = true;
  buttons_.push_back(button);
  return button.id;
}

void UIManager::SetButtonVisible(int buttonId, bool visible) {
  for (auto& button : buttons_) {
    if (button.id == buttonId) {
      button.visible = visible;
      break;
    }
  }
}

void UIManager::SetImageVisible(int imageId, bool visible) {
  for (auto& img : imageElements_) {
    if (img.id == imageId) {
      img.visible = visible;
      break;
    }
  }
}

// 透明區域檢測 - 實際讀取像素數據檢查綠色透明
bool UIManager::IsPointInTransparentArea(int x, int y, const std::wstring& imagePath, const RECT& rect) {
  if (!textureManager_) return false;
  
  // 檢查點是否在矩形內
  if (x < rect.left || x >= rect.right || y < rect.top || y >= rect.bottom) {
    return true; // 在矩形外視為透明
  }
  
  try {
    auto texture = textureManager_->Load(imagePath);
    if (!texture) return false;
    
    IDirect3DTexture9* tex = static_cast<IDirect3DTexture9*>(texture.get());
    D3DSURFACE_DESC desc;
    if (FAILED(tex->GetLevelDesc(0, &desc))) return false;
    
    // 計算紋理座標
    float u = float(x - rect.left) / float(rect.right - rect.left);
    float v = float(y - rect.top) / float(rect.bottom - rect.top);
    
    int texX = int(u * desc.Width);
    int texY = int(v * desc.Height);
    
    if (texX < 0 || texX >= (int)desc.Width || texY < 0 || texY >= (int)desc.Height) {
      return true;
    }
    
    // 實際讀取像素數據來檢查透明度 (只對bg.bmp進行精確檢測)
    if (imagePath == L"bg.bmp") {
      // 獲取surface以讀取像素數據
      ComPtr<IDirect3DSurface9> surface;
      if (SUCCEEDED(tex->GetSurfaceLevel(0, &surface))) {
        D3DLOCKED_RECT lockedRect;
        RECT sourceRect = {texX, texY, texX + 1, texY + 1};
        
        if (SUCCEEDED(surface->LockRect(&lockedRect, &sourceRect, D3DLOCK_READONLY))) {
          // 根據格式讀取像素
          DWORD* pixels = (DWORD*)lockedRect.pBits;
          DWORD pixel = pixels[0];
          
          surface->UnlockRect();
          
          // 檢查是否為綠色 (color key) - 允許一些容差
          BYTE r = (pixel >> 16) & 0xFF;
          BYTE g = (pixel >> 8) & 0xFF;
          BYTE b = pixel & 0xFF;
          
          // 綠色色鍵檢測：綠色較高，紅藍較低
          if (g > 200 && r < 100 && b < 100) {
            return true; // 綠色透明區域
          }
        }
      }
      return false; // 非透明區域
    } 
    else if (imagePath == L"bt.bmp") {
      // bt.bmp使用簡化檢測：邊緣5像素視為透明
      int margin = 5;
      if (x <= rect.left + margin || x >= rect.right - margin ||
          y <= rect.top + margin || y >= rect.bottom - margin) {
        return true;
      }
    }
    
    return false; // 其他區域或文件視為非透明
  } catch (...) {
    return false;
  }
}

int UIManager::GetTopMostElementAt(int x, int y) {
  // 按層級從高到低檢查所有元素
  int topMostId = -1;
  int topMostLayer = -1;
  
  // 檢查按鈕
  for (const auto& button : buttons_) {
    if (!button.visible) continue;
    if (button.layer < (int)layers_.size() && !layers_[button.layer].visible) continue;
    
    if (x >= button.rect.left && x < button.rect.right && 
        y >= button.rect.top && y < button.rect.bottom) {
      if (button.layer > topMostLayer) { // 只考慮更高層級的元素
        if (button.useBackgroundImage) {
          if (!IsPointInTransparentArea(x, y, button.backgroundImage, button.rect)) {
            topMostId = button.id;
            topMostLayer = button.layer;
          }
        } else {
          topMostId = button.id;
          topMostLayer = button.layer;
        }
      }
    }
  }
  
  // 檢查圖片元素
  for (const auto& img : imageElements_) {
    if (!img.visible) continue;
    if (img.layer < (int)layers_.size() && !layers_[img.layer].visible) continue;
    
    if (x >= img.destRect.left && x < img.destRect.right && 
        y >= img.destRect.top && y < img.destRect.bottom) {
      if (img.layer > topMostLayer) { // 只考慮更高層級的元素
        if (!IsPointInTransparentArea(x, y, img.imagePath, img.destRect)) {
          topMostId = img.id;
          topMostLayer = img.layer;
        }
      }
    }
  }
  
  return topMostId;
}

void UIManager::RenderButtons(IDirect3DDevice9* dev) {
  // 移除重複的調試輸出
  
  for (const auto& button : buttons_) {
    if (!button.visible || button.layer >= layers_.size() || !layers_[button.layer].visible) continue;
    
    // 計算最終顏色（考慮層級透明度）
    D3DCOLOR backgroundColor = button.backgroundColor;
    D3DCOLOR textColor = button.textColor;
    
    if (button.layer < layers_.size()) {
      BYTE layerAlpha = BYTE(255 * layers_[button.layer].alpha);
      
      BYTE bgAlpha = BYTE(((backgroundColor >> 24) & 0xFF) * layerAlpha / 255);
      backgroundColor = (backgroundColor & 0x00FFFFFF) | (bgAlpha << 24);
      
      BYTE txtAlpha = BYTE(((textColor >> 24) & 0xFF) * layerAlpha / 255);
      textColor = (textColor & 0x00FFFFFF) | (txtAlpha << 24);
    }
    
    // 渲染按鈕背景
    if (button.useBackgroundImage && textureManager_) {
      auto texture = textureManager_->Load(button.backgroundImage);
      if (texture) {
        D3DXVECTOR3 pos(float(button.rect.left), float(button.rect.top), 0.0f);
        
        // 計算縮放
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
          float scaleX = float(button.rect.right - button.rect.left) / float(desc.Width);
          float scaleY = float(button.rect.bottom - button.rect.top) / float(desc.Height);
          
          D3DXMATRIX scale;
          D3DXMatrixScaling(&scale, scaleX, scaleY, 1.0f);
          sprite_->SetTransform(&scale);
        }
        
        // 根據按鈕狀態調整顏色
        D3DCOLOR finalColor = backgroundColor;
        if (button.isPressed) {
          finalColor = D3DCOLOR_ARGB((finalColor >> 24) & 0xFF, 128, 128, 128); // 變暗
        } else if (button.isHovered) {
          finalColor = D3DCOLOR_ARGB((finalColor >> 24) & 0xFF, 255, 255, 200); // 微亮
        }
        
        sprite_->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
                     nullptr, nullptr, &pos, finalColor);
        
        // 重設變換
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        sprite_->SetTransform(&identity);
      }
    } else {
      // 使用純色背景（簡化實現，可以用矩形繪製）
      // 這裡暫時跳過純色背景的實現
    }
    
    // 渲染按鈕文字
    if (!button.text.empty() && font_) {
      RECT textRect = button.rect;
      // 如果按鈕被按下，文字稍微偏移
      if (button.isPressed) {
        textRect.left += 1;
        textRect.top += 1;
        textRect.right += 1;
        textRect.bottom += 1;
      }
      
      font_->DrawText(sprite_.Get(), button.text.c_str(), -1, &textRect, 
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE, textColor);
    }
  }
}

void UIManager::SortElementsByLayer() {
  std::sort(textElements_.begin(), textElements_.end(),
    [](const UITextElement& a, const UITextElement& b) {
      return a.layer < b.layer;
    });
    
  std::sort(imageElements_.begin(), imageElements_.end(),
    [](const UIImageElement& a, const UIImageElement& b) {
      return a.layer < b.layer;
    });
    
  std::sort(buttons_.begin(), buttons_.end(),
    [](const UIButton& a, const UIButton& b) {
      return a.layer < b.layer;
    });
}

// =============================================================================
// 新的UI組件系統實現
// =============================================================================

// UIImageNew 實現
void UIImageNew::Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) {
  if (!visible || !texMgr) return;
  
  auto texture = texMgr->Load(imagePath);
  if (!texture) return;
  
  RECT absRect = GetAbsoluteRect();
  D3DXVECTOR3 pos(float(absRect.left), float(absRect.top), 0.0f);
  
  // 計算縮放
  D3DSURFACE_DESC desc;
  if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
    float scaleX = float(width) / float(desc.Width);
    float scaleY = float(height) / float(desc.Height);
    
    D3DXMATRIX scale;
    D3DXMatrixScaling(&scale, scaleX, scaleY, 1.0f);
    sprite->SetTransform(&scale);
  }
  
  sprite->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
               nullptr, nullptr, &pos, color);
  
  // 重設變換
  D3DXMATRIX identity;
  D3DXMatrixIdentity(&identity);
  sprite->SetTransform(&identity);
}

bool UIImageNew::OnMouseDown(int x, int y, bool isRightButton) {
  if (!enabled || !visible) return false;
  
  if (isRightButton && draggable) {
    // 開始拖曳
    return true;
  }
  return false;
}

// UIButtonNew 實現  
void UIButtonNew::Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) {
  if (!visible) return;
  
  RECT absRect = GetAbsoluteRect();
  
  // 根據狀態選擇圖片
  std::wstring currentImage;
  switch (state) {
    case State::Hover: currentImage = hoverImage.empty() ? normalImage : hoverImage; break;
    case State::Pressed: currentImage = pressedImage.empty() ? normalImage : pressedImage; break;
    case State::Disabled: currentImage = disabledImage.empty() ? normalImage : disabledImage; break;
    default: currentImage = normalImage; break;
  }
  
  // 渲染背景圖片或純色
  if (!currentImage.empty() && texMgr) {
    auto texture = texMgr->Load(currentImage);
    if (texture) {
      D3DXVECTOR3 pos(float(absRect.left), float(absRect.top), 0.0f);
      
      // 計算縮放
      D3DSURFACE_DESC desc;
      if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
        float scaleX = float(width) / float(desc.Width);
        float scaleY = float(height) / float(desc.Height);
        
        D3DXMATRIX scale;
        D3DXMatrixScaling(&scale, scaleX, scaleY, 1.0f);
        sprite->SetTransform(&scale);
      }
      
      D3DCOLOR btnColor = backgroundColor;
      if (state == State::Pressed) {
        btnColor = D3DCOLOR_ARGB(255, 128, 128, 128); // 變暗
      } else if (state == State::Hover) {
        btnColor = D3DCOLOR_ARGB(255, 255, 255, 200); // 微亮
      }
      
      sprite->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
                   nullptr, nullptr, &pos, btnColor);
      
      // 重設變換
      D3DXMATRIX identity;
      D3DXMatrixIdentity(&identity);
      sprite->SetTransform(&identity);
    }
  } else {
    // 渲染純色背景
    if (dev) {
      struct CUSTOMVERTEX {
        float x, y, z, rhw;
        D3DCOLOR color;
      };
      
      D3DCOLOR btnColor = backgroundColor;
      if (state == State::Pressed) {
        btnColor = D3DCOLOR_ARGB(255, 128, 128, 128); // 變暗
      } else if (state == State::Hover) {
        btnColor = D3DCOLOR_ARGB(255, 220, 220, 220); // 微亮
      }
      
      CUSTOMVERTEX vertices[4] = {
        {float(absRect.left),  float(absRect.top),    0.0f, 1.0f, btnColor},
        {float(absRect.right), float(absRect.top),    0.0f, 1.0f, btnColor},
        {float(absRect.right), float(absRect.bottom), 0.0f, 1.0f, btnColor},
        {float(absRect.left),  float(absRect.bottom), 0.0f, 1.0f, btnColor}
      };
      
      DWORD oldFVF;
      dev->GetFVF(&oldFVF);
      dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
      dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(CUSTOMVERTEX));
      dev->SetFVF(oldFVF);
    }
  }
  
  // 渲染文字 - 需要從UIManager中獲取font
  // 暫時跳過，因為需要重構以傳遞font參數
  // TODO: 重構以支援文字渲染
}

bool UIButtonNew::OnMouseMove(int x, int y) {
  if (!enabled || !visible) return false;
  
  state = State::Hover;
  return true;
}

bool UIButtonNew::OnMouseDown(int x, int y, bool isRightButton) {
  if (!enabled || !visible) return false;
  
  if (!isRightButton) {
    state = State::Pressed;
    return true;
  }
  return false;
}

bool UIButtonNew::OnMouseUp(int x, int y, bool isRightButton) {
  if (!enabled || !visible) return false;
  
  if (!isRightButton && state == State::Pressed) {
    state = State::Normal;
    if (onClick) {
      onClick();
    }
    return true;
  }
  return false;
}

// UIEditNew 實現
void UIEditNew::Render(IDirect3DDevice9* dev, ID3DXSprite* sprite, ITextureManager* texMgr) {
  if (!visible) return;
  
  RECT absRect = GetAbsoluteRect();
  
  // 渲染背景圖片或純色
  if (!backgroundImage.empty() && texMgr) {
    auto texture = texMgr->Load(backgroundImage);
    if (texture) {
      D3DXVECTOR3 pos(float(absRect.left), float(absRect.top), 0.0f);
      
      D3DSURFACE_DESC desc;
      if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
        float scaleX = float(width) / float(desc.Width);
        float scaleY = float(height) / float(desc.Height);
        
        D3DXMATRIX scale;
        D3DXMatrixScaling(&scale, scaleX, scaleY, 1.0f);
        sprite->SetTransform(&scale);
      }
      
      sprite->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
                   nullptr, nullptr, &pos, backgroundColor);
      
      D3DXMATRIX identity;
      D3DXMatrixIdentity(&identity);
      sprite->SetTransform(&identity);
    }
  } else {
    // 沒有背景圖片時，渲染純色矩形（使用D3D繪製）
    if (dev) {
      // 創建矩形頂點
      struct CUSTOMVERTEX {
        float x, y, z, rhw;
        D3DCOLOR color;
      };
      
      CUSTOMVERTEX vertices[4] = {
        {float(absRect.left),  float(absRect.top),    0.0f, 1.0f, backgroundColor},
        {float(absRect.right), float(absRect.top),    0.0f, 1.0f, backgroundColor},
        {float(absRect.right), float(absRect.bottom), 0.0f, 1.0f, backgroundColor},
        {float(absRect.left),  float(absRect.bottom), 0.0f, 1.0f, backgroundColor}
      };
      
      // 保存原始狀態
      DWORD oldFVF;
      dev->GetFVF(&oldFVF);
      
      // 設定FVF和頂點格式
      dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
      dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(CUSTOMVERTEX));
      
      // 恢復FVF
      dev->SetFVF(oldFVF);
      
      // 渲染邊框
      if (isFocused) {
        CUSTOMVERTEX border[5] = {
          {float(absRect.left),  float(absRect.top),    0.0f, 1.0f, borderColor},
          {float(absRect.right), float(absRect.top),    0.0f, 1.0f, borderColor},
          {float(absRect.right), float(absRect.bottom), 0.0f, 1.0f, borderColor},
          {float(absRect.left),  float(absRect.bottom), 0.0f, 1.0f, borderColor},
          {float(absRect.left),  float(absRect.top),    0.0f, 1.0f, borderColor}
        };
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        dev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, border, sizeof(CUSTOMVERTEX));
        dev->SetFVF(oldFVF);
      }
    }
  }
  
  // TODO: 渲染邊框、文字和游標
  // 需要font參數才能渲染文字
}

bool UIEditNew::OnMouseDown(int x, int y, bool isRightButton) {
  if (!enabled || !visible) return false;
  
  if (!isRightButton) {
    isFocused = true;
    // TODO: 計算游標位置
    return true;
  }
  return false;
}

bool UIEditNew::OnKeyDown(WPARAM key) {
  if (!isFocused || !enabled) return false;
  
  switch (key) {
    case VK_LEFT:
      if (cursorPos > 0) cursorPos--;
      return true;
    case VK_RIGHT:
      if (cursorPos < (int)text.length()) cursorPos++;
      return true;
    case VK_HOME:
      cursorPos = 0;
      return true;
    case VK_END:
      cursorPos = (int)text.length();
      return true;
    case VK_BACK:
      if (cursorPos > 0) {
        text.erase(cursorPos - 1, 1);
        cursorPos--;
      }
      return true;
    case VK_DELETE:
      if (cursorPos < (int)text.length()) {
        text.erase(cursorPos, 1);
      }
      return true;
  }
  return false;
}

bool UIEditNew::OnChar(WPARAM ch) {
  if (!isFocused || !enabled) return false;
  
  // 支援Unicode字元輸入
  if (ch >= 32 && ch != 127) { // 可顯示字元
    if ((int)text.length() < maxLength) {
      text.insert(cursorPos, 1, (wchar_t)ch);
      cursorPos++;
      return true;
    }
  }
  return false;
}

// =============================================================================
// UIManager 新組件系統方法實現
// =============================================================================

UIComponentNew* UIManager::CreateImage(const std::wstring& imagePath, int x, int y, int width, int height, 
                                       bool draggable, UIComponentNew* parent) {
  auto image = std::make_unique<UIImageNew>();
  image->id = nextId_++;
  image->imagePath = imagePath;
  image->relativeX = x;
  image->relativeY = y;
  image->width = width;
  image->height = height;
  image->draggable = draggable;
  image->parent = parent;
  
  UIComponentNew* result = image.get();
  
  if (parent) {
    parent->children.push_back(std::move(image));
  } else {
    rootComponents_.push_back(std::move(image));
  }
  
  return result;
}

UIComponentNew* UIManager::CreateButton(const std::wstring& text, int x, int y, int width, int height,
                                        std::function<void()> onClick, UIComponentNew* parent,
                                    const std::wstring& normalImage,
                                    const std::wstring& hoverImage, 
                                    const std::wstring& pressedImage, 
                                    const std::wstring& disabledImage) {
  auto button = std::make_unique<UIButtonNew>();
  button->id = nextId_++;
  button->text = text;
  button->relativeX = x;
  button->relativeY = y;
  button->width = width;
  button->height = height;
  button->onClick = onClick;
  button->parent = parent;
  button->normalImage = normalImage;
  button->hoverImage = hoverImage;
  button->pressedImage = pressedImage;
  button->disabledImage = disabledImage;
  
  UIComponentNew* result = button.get();
  
  if (parent) {
    parent->children.push_back(std::move(button));
  } else {
    rootComponents_.push_back(std::move(button));
  }
  
  return result;
}

UIComponentNew* UIManager::CreateEdit(int x, int y, int width, int height, UIComponentNew* parent,
                                  const std::wstring& backgroundImage) {
  auto edit = std::make_unique<UIEditNew>();
  edit->id = nextId_++;
  edit->relativeX = x;
  edit->relativeY = y;
  edit->width = width;
  edit->height = height;
  edit->parent = parent;
  edit->backgroundImage = backgroundImage;
  
  UIComponentNew* result = edit.get();
  
  if (parent) {
    parent->children.push_back(std::move(edit));
  } else {
    rootComponents_.push_back(std::move(edit));
  }
  
  return result;
}

// 智能事件委派 - 遞歸查找滑鼠下的組件，考慮透明度
UIComponentNew* UIManager::GetComponentAt(int x, int y) {
  std::function<UIComponentNew*(const std::vector<std::unique_ptr<UIComponentNew>>&)> findComponent;
  
  findComponent = [&](const std::vector<std::unique_ptr<UIComponentNew>>& components) -> UIComponentNew* {
    // 反向遍歷，因為後添加的在上層
    for (auto it = components.rbegin(); it != components.rend(); ++it) {
      auto& comp = *it;
      if (!comp->visible) continue;
      
      RECT rect = comp->GetAbsoluteRect();
      if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
        // 先檢查子組件
        UIComponentNew* child = findComponent(comp->children);
        if (child) return child;
        
        // 檢查當前組件是否在透明區域 (只對圖片組件檢查)
        if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
          if (img->useTransparency && IsPointInTransparentArea(x, y, img->imagePath, rect)) {
            continue; // 在透明區域，跳過此組件
          }
        }
        
        // 沒有子組件被點擊且不在透明區域，返回此組件
        return comp.get();
      }
    }
    return nullptr;
  };
  
  return findComponent(rootComponents_);
}

void UIManager::SetFocusedComponent(UIComponentNew* component) {
  // 清除舊的焦點
  if (focusedComponent_) {
    if (auto* edit = dynamic_cast<UIEditNew*>(focusedComponent_)) {
      edit->isFocused = false;
    }
  }
  
  focusedComponent_ = component;
  
  // 設置新的焦點
  if (focusedComponent_) {
    if (auto* edit = dynamic_cast<UIEditNew*>(focusedComponent_)) {
      edit->isFocused = true;
    }
  }
}

void UIManager::RenderComponents(IDirect3DDevice9* dev, const std::vector<std::unique_ptr<UIComponentNew>>& components) {
  for (const auto& comp : components) {
    if (comp->visible) {
      comp->Render(dev, sprite_.Get(), textureManager_);
      
      // 遞歸渲染子組件
      RenderComponents(dev, comp->children);
    }
  }
}