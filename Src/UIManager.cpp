#include "UIManager.h"
#include <algorithm>
#include <string>
#include <windowsx.h>  // For GET_X_LPARAM and GET_Y_LPARAM
#include "UICoordinateFix.h"

// Factory 函式實作
std::unique_ptr<IUIManager> CreateUIManager(ITextureManager* textureManager) {
  return std::make_unique<UIManager>(textureManager);
}

UIManager::UIManager(ITextureManager* textureManager) 
  : textureManager_(textureManager) {
  // 創建預設層 (layer 0)
  CreateLayer(L"Default", 0.0f, 1.0f);
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
  
  // 設定alpha混合 - 使用正確的 alpha 混合模式
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  
  // 關閉 Z 緩衝區以避免 UI 問題
  dev->SetRenderState(D3DRS_ZENABLE, FALSE);
  
  // 設定顏色鍵 (Color Key) - 綠色透明
  // 注意: D3DXSprite 不直接支援顏色鍵，需要在載入紋理時處理
  
  // 不要設定任何變換矩陣給 D3D 裝置 - 只在 2D UI 中使用
  // 文字使用的是螢幕空間座標，sprite 也應該一樣
  
  // 設定alpha混合狀態 - 不要排序以避免問題
  sprite_->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DONOTSAVESTATE);
  
  // 在 Begin 之後設定單位矩陣
  D3DXMATRIX identityMatrix;
  D3DXMatrixIdentity(&identityMatrix);
  sprite_->SetTransform(&identityMatrix);
  
  // 診斷輸出視窗和視圖設定
  static bool diagnosticShown = false;
  if (!diagnosticShown) {
    diagnosticShown = true;
    
    // 獲取當前的變換矩陣
    D3DXMATRIX currentTransform;
    sprite_->GetTransform(&currentTransform);
    
    char debugMsg[512];
    sprintf_s(debugMsg, "Sprite Transform Matrix:\n  [%.3f, %.3f, %.3f, %.3f]\n  [%.3f, %.3f, %.3f, %.3f]\n  [%.3f, %.3f, %.3f, %.3f]\n  [%.3f, %.3f, %.3f, %.3f]\n",
              currentTransform._11, currentTransform._12, currentTransform._13, currentTransform._14,
              currentTransform._21, currentTransform._22, currentTransform._23, currentTransform._24,
              currentTransform._31, currentTransform._32, currentTransform._33, currentTransform._34,
              currentTransform._41, currentTransform._42, currentTransform._43, currentTransform._44);
    OutputDebugStringA(debugMsg);
    
    // 獲取視圖設定
    D3DVIEWPORT9 viewport;
    if (SUCCEEDED(dev->GetViewport(&viewport))) {
      char debugMsg[256];
      sprintf_s(debugMsg, "D3D Viewport: X=%d, Y=%d, Width=%d, Height=%d\n",
                viewport.X, viewport.Y, viewport.Width, viewport.Height);
      OutputDebugStringA(debugMsg);
    }
    
    // 獲取變換矩陣
    D3DXMATRIX world, view, proj;
    dev->GetTransform(D3DTS_WORLD, &world);
    dev->GetTransform(D3DTS_VIEW, &view);
    dev->GetTransform(D3DTS_PROJECTION, &proj);
    
    OutputDebugStringA("Transform Matrices:\n");
    
    // 檢查是否為單位矩陣
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    
    if (memcmp(&world, &identity, sizeof(D3DXMATRIX)) == 0) {
      OutputDebugStringA("  World: Identity\n");
    } else {
      OutputDebugStringA("  World: Modified\n");
    }
    
    if (memcmp(&view, &identity, sizeof(D3DXMATRIX)) == 0) {
      OutputDebugStringA("  View: Identity\n");
    } else {
      OutputDebugStringA("  View: Modified\n");
    }
    
    if (memcmp(&proj, &identity, sizeof(D3DXMATRIX)) == 0) {
      OutputDebugStringA("  Projection: Identity\n");
    } else {
      OutputDebugStringA("  Projection: Modified - This could cause UI coordinate issues!\n");
      
      // 輸出投影矩陣的值
      char matrixDebug[1024];
      sprintf_s(matrixDebug, "  Projection Matrix:\n");
      OutputDebugStringA(matrixDebug);
      for (int i = 0; i < 4; i++) {
        sprintf_s(matrixDebug, "    [%.3f, %.3f, %.3f, %.3f]\n",
                  proj.m[i][0], proj.m[i][1], proj.m[i][2], proj.m[i][3]);
        OutputDebugStringA(matrixDebug);
      }
    }
  }
  
  // 不要修改 D3D 變換矩陣 - Sprite 在螢幕空間工作
  
  // 渲染圖片元素
  for (const auto& img : imageElements_) {
    if (!img.visible || img.layer >= layers_.size() || !layers_[img.layer].visible) continue;
    
    if (textureManager_) {
      auto texture = textureManager_->Load(img.imagePath);
      if (texture) {
        // 診斷特定圖片的座標
        if (img.imagePath == L"bg.bmp") {
          static int frameCount = 0;
          if (frameCount++ % 60 == 0) { // 每秒一次
            char debugMsg[512];
            sprintf_s(debugMsg, "Render bg.bmp Legacy: destRect=(%d,%d)-(%d,%d), pos=(%.1f,%.1f)\n",
                      img.destRect.left, img.destRect.top, img.destRect.right, img.destRect.bottom,
                      float(img.destRect.left), float(img.destRect.top));
            OutputDebugStringA(debugMsg);
            
            // 測試座標轉換
            D3DXVECTOR3 testPos(100.0f, 100.0f, 0.0f);
            D3DXVECTOR4 transformed;
            D3DXMATRIX currentTransform;
            sprite_->GetTransform(&currentTransform);
            D3DXVec3Transform(&transformed, &testPos, &currentTransform);
            sprintf_s(debugMsg, "  Test transform (100,100) -> (%.1f,%.1f)\n", transformed.x, transformed.y);
            OutputDebugStringA(debugMsg);
          }
        }
        
        D3DXVECTOR3 pos(float(img.destRect.left), float(img.destRect.top), 0.0f);
        
        // 計算縮放
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture.get())->GetLevelDesc(0, &desc))) {
          float scaleX = float(img.destRect.right - img.destRect.left) / float(desc.Width);
          float scaleY = float(img.destRect.bottom - img.destRect.top) / float(desc.Height);
          
          // 暫時不使用縮放 - 直接以原始大小顯示
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
  // 使用 GET_X_LPARAM 和 GET_Y_LPARAM 正確獲取滑鼠座標
  // 這些宏能正確處理多螢幕和高DPI情況
  int mouseX = GET_X_LPARAM(msg.lParam);
  int mouseY = GET_Y_LPARAM(msg.lParam);
  
  switch (msg.message) {
    case WM_MOUSEMOVE: {
      // 優先處理拖曳 - 只處理根組件拖曳
      if (draggedComponent_ && isDragging_) {
        // 計算新的絕對位置（滑鼠位置 - 拖曳偏移）
        int newAbsX = mouseX - dragOffset_.x;
        int newAbsY = mouseY - dragOffset_.y;
        
        // 由於draggedComponent_必須是根組件（沒有parent），直接設定相對座標
        int newRelativeX = newAbsX;
        int newRelativeY = newAbsY;
        
        // 計算移動的差值
        int deltaX = newRelativeX - draggedComponent_->relativeX;
        int deltaY = newRelativeY - draggedComponent_->relativeY;
        
        // 更詳細的調試輸出
        static int dragUpdateCount = 0;
        if (dragUpdateCount++ % 10 == 0) { // 每10次更新輸出一次
          OutputDebugStringA(("Drag details: mouse=(" + std::to_string(mouseX) + "," + std::to_string(mouseY) + 
                             "), offset=(" + std::to_string(dragOffset_.x) + "," + std::to_string(dragOffset_.y) +
                             "), oldPos=(" + std::to_string(draggedComponent_->relativeX) + "," + std::to_string(draggedComponent_->relativeY) +
                             "), newPos=(" + std::to_string(newRelativeX) + "," + std::to_string(newRelativeY) +
                             "), delta=(" + std::to_string(deltaX) + "," + std::to_string(deltaY) + ")\n").c_str());
        }
        
        // 更新位置（只有位置真的改變時才更新）
        if (deltaX != 0 || deltaY != 0) {
          draggedComponent_->relativeX = newRelativeX;
          draggedComponent_->relativeY = newRelativeY;
        }
        
        return true; // 拖曳中，攔截所有滑鼠移動事件
      }
      
      // 沒有在拖曳時才處理 hover 狀態
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
      
      return component != nullptr; // 如果滑鼠在UI上，阻止相機處理
    }
    
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      UIComponentNew* component = GetComponentAt(mouseX, mouseY);
      
      // 調試輸出 - 也輸出左鍵點擊的信息
      if (msg.message == WM_LBUTTONDOWN) {
        if (component) {
          std::string compInfo = "unknown";
          if (auto* img = dynamic_cast<UIImageNew*>(component)) {
            compInfo = std::string(img->imagePath.begin(), img->imagePath.end());
          } else if (auto* btn = dynamic_cast<UIButtonNew*>(component)) {
            compInfo = "Button: " + std::string(btn->text.begin(), btn->text.end());
          }
          OutputDebugStringA(("LEFT click - GetComponentAt found " + compInfo + " at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
        } else {
          OutputDebugStringA(("LEFT click - GetComponentAt found NO component at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
        }
      }
      
      // 調試輸出
      if (msg.message == WM_RBUTTONDOWN) {
        if (component) {
          OutputDebugStringA(("RIGHT click - GetComponentAt found component at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
        } else {
          OutputDebugStringA(("RIGHT click - GetComponentAt found NO component at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
          // 輸出所有根組件的位置以診斷問題
          OutputDebugStringA("  Current root components:\n");
          for (const auto& comp : rootComponents_) {
            if (comp->visible) {
              RECT rect = comp->GetAbsoluteRect();
              if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
                OutputDebugStringA(("    " + std::string(img->imagePath.begin(), img->imagePath.end()) + 
                                   " at (" + std::to_string(rect.left) + "," + std::to_string(rect.top) + 
                                   ")-(" + std::to_string(rect.right) + "," + std::to_string(rect.bottom) + 
                                   "), transparent=" + std::string(img->useTransparency ? "YES" : "NO") + "\n").c_str());
              }
            }
          }
        }
      }
      
      if (component) {
        bool isRightButton = (msg.message == WM_RBUTTONDOWN);
        bool handled = component->OnMouseDown(mouseX, mouseY, isRightButton);
        
        // 設定焦點
        SetFocusedComponent(component);
        
        // 檢查是否開始拖曳
        if (isRightButton) {
          // 使用特殊的方法來獲取可拖曳組件，這個方法會忽略透明度檢查
          UIComponentNew* draggableComponent = GetDraggableComponentAt(mouseX, mouseY);
          
          // 調試輸出
          if (draggableComponent) {
            std::string compInfo = "unknown";
            if (auto* img = dynamic_cast<UIImageNew*>(draggableComponent)) {
              compInfo = std::string(img->imagePath.begin(), img->imagePath.end());
            }
            OutputDebugStringA(("GetDraggableComponentAt found: " + compInfo + " at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
          } else {
            OutputDebugStringA(("GetDraggableComponentAt found NO component at (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ")\n").c_str());
          }
          
          if (draggableComponent) {
            // 找到可拖曳的根組件
            UIComponentNew* draggableRoot = draggableComponent;
            while (draggableRoot && draggableRoot->parent) {
              draggableRoot = draggableRoot->parent;
            }
            
            // 檢查根組件是否可拖曳
            if (auto* rootImg = dynamic_cast<UIImageNew*>(draggableRoot)) {
              if (rootImg->draggable) {
                RECT rootRect = draggableRoot->GetAbsoluteRect();
                
                draggedComponent_ = draggableRoot;
                
                // 計算拖曳偏移 - 滑鼠位置相對於根組件左上角的偏移
                dragOffset_.x = mouseX - rootRect.left;
                dragOffset_.y = mouseY - rootRect.top;
                
                SetCapture(msg.hwnd); // 捕獲滑鼠，確保拖曳期間UI有完全控制權
                isDragging_ = true;
                
                // 輸出調試信息
                OutputDebugStringA(("Start dragging root component at (" + 
                                   std::to_string(draggableRoot->relativeX) + "," + 
                                   std::to_string(draggableRoot->relativeY) + "), offset=(" +
                                   std::to_string(dragOffset_.x) + "," + 
                                   std::to_string(dragOffset_.y) + ")\n").c_str());
              }
            }
          }
          
          if (!isDragging_) {
            OutputDebugStringA("  No draggable component found\n");
          }
        }
        
        return handled;
      } else {
        // 點擊空白區域，清除焦點
        SetFocusedComponent(nullptr);
        return false; // 讓其他系統處理
      }
      break;
    }
    
    case WM_LBUTTONUP:
    case WM_RBUTTONUP: {
      if (draggedComponent_ && isDragging_) {
        OutputDebugStringA(("End dragging at (" + 
                           std::to_string(draggedComponent_->relativeX) + "," + 
                           std::to_string(draggedComponent_->relativeY) + ")\n").c_str());
        draggedComponent_ = nullptr;
        dragOffset_ = {0, 0};  // 重置拖曳偏移
        isDragging_ = false;   // 重置拖曳狀態
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

int UIManager::CreateLayer(const std::wstring& name, float priority, float alpha) {
  UILayerLegacy layer;
  layer.name = name;
  layer.alpha = alpha;
  layer.priority = priority;
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

int UIManager::AddText(const std::wstring& text, int x, int y, int width, int height, 
                       unsigned long color, int layer) {
  static int nextTextId = 1;
  UITextElement element;
  element.id = nextTextId++;
  element.text = text;
  element.rect = {x, y, x + width, y + height};
  element.color = color;
  element.format = DT_LEFT | DT_TOP;
  element.layer = layer;
  textElements_.push_back(element);
  return element.id;
}

void UIManager::UpdateText(int textId, const std::wstring& newText) {
  auto it = std::find_if(textElements_.begin(), textElements_.end(),
    [textId](const UITextElement& elem) { return elem.id == textId; });
  
  if (it != textElements_.end()) {
    it->text = newText;
  }
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

// 透明區域檢測 - 使用快取的 alpha 遮罩
bool UIManager::IsPointInTransparentArea(int x, int y, const std::wstring& imagePath, const RECT& rect) {
  if (!textureManager_) return false;
  
  // 檢查點是否在矩形內
  if (x < rect.left || x >= rect.right || y < rect.top || y >= rect.bottom) {
    return true; // 在矩形外視為透明
  }
  
  // 檢查是否有快取的 alpha 遮罩
  auto it = alphaMaskCache_.find(imagePath);
  if (it == alphaMaskCache_.end()) {
    // 建立 alpha 遮罩
    BuildAlphaMask(imagePath);
    it = alphaMaskCache_.find(imagePath);
    if (it == alphaMaskCache_.end()) {
      return false; // 無法建立遮罩，視為不透明
    }
  }
  
  const AlphaMask& mask = it->second;
  
  // 計算紋理座標
  float u = float(x - rect.left) / float(rect.right - rect.left);
  float v = float(y - rect.top) / float(rect.bottom - rect.top);
  
  int texX = int(u * mask.width);
  int texY = int(v * mask.height);
  
  if (texX < 0 || texX >= mask.width || texY < 0 || texY >= mask.height) {
    return true;
  }
  
  // 查詢遮罩
  int index = texY * mask.width + texX;
  return !mask.mask[index]; // false = 透明, true = 不透明
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
  // Sort by layer priority instead of layer ID
  std::sort(textElements_.begin(), textElements_.end(),
    [this](const UITextElement& a, const UITextElement& b) {
      if (a.layer < layers_.size() && b.layer < layers_.size()) {
        return layers_[a.layer].priority < layers_[b.layer].priority;
      }
      return a.layer < b.layer;
    });
    
  std::sort(imageElements_.begin(), imageElements_.end(),
    [this](const UIImageElement& a, const UIImageElement& b) {
      if (a.layer < layers_.size() && b.layer < layers_.size()) {
        return layers_[a.layer].priority < layers_[b.layer].priority;
      }
      return a.layer < b.layer;
    });
    
  std::sort(buttons_.begin(), buttons_.end(),
    [this](const UIButton& a, const UIButton& b) {
      if (a.layer < layers_.size() && b.layer < layers_.size()) {
        return layers_[a.layer].priority < layers_[b.layer].priority;
      }
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
  
  // 調試輸出 - 關閉過於頻繁的渲染調試
  // if (imagePath == L"bg.png") {
  //   static int frameCount = 0;
  //   if (frameCount++ % 600 == 0) { // 每10秒一次
  //     std::wstring debugMsg = L"Render bg.png: relative=(" + std::to_wstring(relativeX) + L"," + std::to_wstring(relativeY) + 
  //                            L"), absolute=(" + std::to_wstring(absRect.left) + L"," + std::to_wstring(absRect.top) + 
  //                            L"), size=(" + std::to_wstring(width) + L"x" + std::to_wstring(height) + L")\n";
  //     OutputDebugStringW(debugMsg.c_str());
  //   }
  // }
  
  // 計算座標位置 - 檢查是否需要偏移調整
  float renderX = float(absRect.left);
  float renderY = float(absRect.top);
  
  // 診斷輸出座標轉換和實際紋理大小 - 已關閉
  // if (imagePath == L"bg.png") {
  //   static int diagCount = 0;
  //   if (diagCount++ < 5) { // 只輸出前5次
  //     char debugMsg[512];
  //     sprintf_s(debugMsg, "UIImageNew::Render bg.png: relativeXY=(%d,%d), absRect=(%d,%d,%d,%d), renderPos=(%.1f,%.1f)\n",
  //               relativeX, relativeY,
  //               absRect.left, absRect.top, absRect.right, absRect.bottom,
  //               renderX, renderY);
  //     OutputDebugStringA(debugMsg);
  //     
  //     // 也輸出父元素資訊
  //     if (parent) {
  //       RECT parentRect = parent->GetAbsoluteRect();
  //       sprintf_s(debugMsg, "  Parent: absRect=(%d,%d,%d,%d)\n",
  //                 parentRect.left, parentRect.top, parentRect.right, parentRect.bottom);
  //       OutputDebugStringA(debugMsg);
  //     } else {
  //       OutputDebugStringA("  Parent: None (root element)\n");
  //     }
  //   }
  // }
  
  D3DXVECTOR3 pos(renderX, renderY, 0.0f);
  
  // 檢查實際紋理大小
  if (imagePath == L"bg.png") {
    IDirect3DTexture9* tex = static_cast<IDirect3DTexture9*>(texture.get());
    D3DSURFACE_DESC desc;
    if (SUCCEEDED(tex->GetLevelDesc(0, &desc))) {
      static bool sizeReported = false;
      if (!sizeReported) {
        sizeReported = true;
        char debugMsg[256];
        sprintf_s(debugMsg, "bg.png actual texture size: %dx%d, component size: %dx%d\n", 
                  desc.Width, desc.Height, width, height);
        OutputDebugStringA(debugMsg);
      }
    }
  }
  
  // 直接以原始大小繪製，不進行縮放
  sprite->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
               nullptr, nullptr, &pos, color);
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
      
      // 暫時不使用縮放 - 先確認座標正確
      
      D3DCOLOR btnColor = backgroundColor;
      if (state == State::Pressed) {
        btnColor = D3DCOLOR_ARGB(255, 128, 128, 128); // 變暗
      } else if (state == State::Hover) {
        btnColor = D3DCOLOR_ARGB(255, 255, 255, 200); // 微亮
      }
      
      sprite->Draw(static_cast<IDirect3DTexture9*>(texture.get()), 
                   nullptr, nullptr, &pos, btnColor);
      
      // 不需要重設變換
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

// 輔助函數 - 獲取圖片尺寸
bool UIManager::GetImageSize(const std::wstring& imagePath, int& width, int& height) const {
  if (!textureManager_) return false;
  
  auto texture = textureManager_->Load(imagePath);
  if (!texture) return false;
  
  IDirect3DTexture9* tex = static_cast<IDirect3DTexture9*>(texture.get());
  D3DSURFACE_DESC desc;
  if (FAILED(tex->GetLevelDesc(0, &desc))) return false;
  
  width = desc.Width;
  height = desc.Height;
  return true;
}

UIComponentNew* UIManager::CreateImage(const std::wstring& imagePath, int x, int y, int width, int height, 
                                       bool draggable, UIComponentNew* parent, bool allowDragFromTransparent) {
  auto image = std::make_unique<UIImageNew>();
  image->id = nextId_++;
  image->imagePath = imagePath;
  
  // 從圖片路徑提取檔案名稱作為組件名稱
  size_t lastSlash = imagePath.find_last_of(L"/\\");
  if (lastSlash != std::wstring::npos) {
    image->name = imagePath.substr(lastSlash + 1);
  } else {
    image->name = imagePath;
  }
  
  image->relativeX = x;
  image->relativeY = y;
  image->width = width;
  image->height = height;
  image->draggable = draggable;
  image->parent = parent;
  image->allowDragFromTransparent = allowDragFromTransparent;
  image->manager = this;  // 設置管理器指針
  
  // 對於圖片組件，默認啟用透明度檢測
  // 但拖曳操作會根據 allowDragFromTransparent 參數決定是否允許在透明區域拖曳
  image->useTransparency = true;
  
  // 調試輸出
  std::wstring debugMsg = L"CreateImage: " + imagePath + L" at (" + std::to_wstring(x) + L"," + std::to_wstring(y) + L")";
  if (parent) {
    debugMsg += L" [child of parent]";
  }
  if (draggable && !parent) {
    debugMsg += L" [draggable root - transparency DISABLED]";
  }
  debugMsg += L"\n";
  OutputDebugStringW(debugMsg.c_str());
  
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
  // 調試輸出：追蹤按鈕創建
  OutputDebugStringA(("UIManager::CreateButton - Creating button: " + 
                     std::string(text.begin(), text.end()) + 
                     " at (" + std::to_string(x) + "," + std::to_string(y) + ")\n").c_str());
  
  auto button = std::make_unique<UIButtonNew>();
  button->id = nextId_++;
  button->text = text;
  button->name = L"Button_" + text;  // 使用按鈕文字作為名稱
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
  button->manager = this;  // 設置管理器指針
  
  // 包裝原始的onClick，加入事件通知
  auto originalOnClick = onClick;
  auto* manager = this;
  auto* btnPtr = button.get();
  button->onClick = [originalOnClick, manager, btnPtr]() {
    if (originalOnClick) {
      originalOnClick();
    }
    // 通知監聽器
    manager->NotifyButtonClicked(btnPtr);
  };
  
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
  edit->name = L"Edit_" + std::to_wstring(nextId_ - 1);  // 使用ID作為名稱的一部分
  edit->relativeX = x;
  edit->relativeY = y;
  edit->width = width;
  edit->height = height;
  edit->parent = parent;
  edit->backgroundImage = backgroundImage;
  edit->manager = this;  // 設置管理器指針
  
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
      
      // 對 bg.png 添加特殊調試
      if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
        if (img->imagePath == L"bg.png") {
          static int missCount = 0;
          if (missCount++ < 5) { // 只輸出前5次
            OutputDebugStringA(("  Checking bg.png: point(" + std::to_string(x) + "," + std::to_string(y) + 
                               ") vs rect(" + std::to_string(rect.left) + "," + std::to_string(rect.top) + 
                               ")-(" + std::to_string(rect.right) + "," + std::to_string(rect.bottom) + ")\n").c_str());
            
            // 分析為什麼不在範圍內
            if (y < rect.top) {
              OutputDebugStringA(("    -> Mouse is ABOVE component (y=" + std::to_string(y) + 
                                 " < top=" + std::to_string(rect.top) + ")\n").c_str());
            }
            if (y >= rect.bottom) {
              OutputDebugStringA(("    -> Mouse is BELOW component (y=" + std::to_string(y) + 
                                 " >= bottom=" + std::to_string(rect.bottom) + ")\n").c_str());
            }
            if (x < rect.left) {
              OutputDebugStringA(("    -> Mouse is LEFT of component (x=" + std::to_string(x) + 
                                 " < left=" + std::to_string(rect.left) + ")\n").c_str());
            }
            if (x >= rect.right) {
              OutputDebugStringA(("    -> Mouse is RIGHT of component (x=" + std::to_string(x) + 
                                 " >= right=" + std::to_string(rect.right) + ")\n").c_str());
            }
          }
        }
      }
      
      if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
        // 先檢查子組件
        UIComponentNew* child = findComponent(comp->children);
        if (child) return child;
        
        // 檢查當前組件是否在透明區域 (只對圖片組件檢查)
        if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
          if (img->useTransparency && IsPointInTransparentArea(x, y, img->imagePath, rect)) {
            // 為 b-kuang.png 添加特殊調試
            if (img->imagePath == L"b-kuang.png") {
              OutputDebugStringA(("b-kuang.png: Point (" + std::to_string(x) + "," + std::to_string(y) + 
                                 ") is in TRANSPARENT area, skipping to allow click-through\n").c_str());
            }
            continue; // 在透明區域，跳過此組件
          } else if (img->imagePath == L"b-kuang.png") {
            OutputDebugStringA(("b-kuang.png: Point (" + std::to_string(x) + "," + std::to_string(y) + 
                               ") is in NON-TRANSPARENT area\n").c_str());
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

UIComponentNew* UIManager::GetDraggableComponentAt(int x, int y) {
  
  std::function<UIComponentNew*(const std::vector<std::unique_ptr<UIComponentNew>>&)> findComponent;
  
  findComponent = [&](const std::vector<std::unique_ptr<UIComponentNew>>& components) -> UIComponentNew* {
    // 反向遍歷，因為後添加的在上層
    for (auto it = components.rbegin(); it != components.rend(); ++it) {
      auto& comp = *it;
      if (!comp->visible) continue;
      
      RECT rect = comp->GetAbsoluteRect();
      
      if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
        
        // 對於可拖曳的根組件，根據 allowDragFromTransparent 決定是否檢查透明度
        if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
          if (img->draggable && !img->parent) {
            // 如果不允許從透明區域拖曳，則需要檢查透明度
            if (!img->allowDragFromTransparent && img->useTransparency && 
                IsPointInTransparentArea(x, y, img->imagePath, rect)) {
              // 調試輸出
              OutputDebugStringA(("GetDraggableComponentAt: " + 
                                 std::string(img->imagePath.begin(), img->imagePath.end()) + 
                                 " - point in transparent area, drag not allowed\n").c_str());
              continue; // 在透明區域且不允許從透明區域拖曳，跳過
            }
            
            // 調試輸出
            OutputDebugStringA(("GetDraggableComponentAt: Found draggable root " + 
                               std::string(img->imagePath.begin(), img->imagePath.end()) + 
                               " at (" + std::to_string(x) + "," + std::to_string(y) + 
                               "), allowDragFromTransparent=" + (img->allowDragFromTransparent ? "true" : "false") + "\n").c_str());
            return comp.get();
          }
        }
        
        // 先檢查子組件
        UIComponentNew* child = findComponent(comp->children);
        if (child) return child;
        
        // 如果不是可拖曳的根組件，則檢查透明度
        if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
          if (img->useTransparency && IsPointInTransparentArea(x, y, img->imagePath, rect)) {
            // 調試輸出
            OutputDebugStringA(("GetDraggableComponentAt: Skipping " + 
                               std::string(img->imagePath.begin(), img->imagePath.end()) + 
                               " due to transparency at (" + std::to_string(x) + "," + std::to_string(y) + ")\n").c_str());
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

// 建立 Alpha 遮罩
void UIManager::BuildAlphaMask(const std::wstring& imagePath) const {
  if (!textureManager_) return;
  
  try {
    auto texture = textureManager_->Load(imagePath);
    if (!texture) return;
    
    IDirect3DTexture9* tex = static_cast<IDirect3DTexture9*>(texture.get());
    D3DSURFACE_DESC desc;
    if (FAILED(tex->GetLevelDesc(0, &desc))) return;
    
    AlphaMask mask;
    mask.width = desc.Width;
    mask.height = desc.Height;
    mask.mask.resize(desc.Width * desc.Height, true); // 預設為不透明
    
    // 獲取surface以讀取像素數據
    ComPtr<IDirect3DSurface9> surface;
    if (SUCCEEDED(tex->GetSurfaceLevel(0, &surface))) {
      D3DLOCKED_RECT lockedRect;
      
      if (SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY))) {
        // 判斷是 BMP 還是 PNG
        bool isBMP = imagePath.find(L".bmp") != std::wstring::npos;
        bool isPNG = imagePath.find(L".png") != std::wstring::npos;
        
        // 根據格式讀取像素
        for (UINT y = 0; y < desc.Height; y++) {
          DWORD* row = (DWORD*)((BYTE*)lockedRect.pBits + y * lockedRect.Pitch);
          
          for (UINT x = 0; x < desc.Width; x++) {
            DWORD pixel = row[x];
            
            bool isTransparent = false;
            
            if (isBMP) {
              // BMP: 檢查綠色色鍵
              BYTE r = (pixel >> 16) & 0xFF;
              BYTE g = (pixel >> 8) & 0xFF;
              BYTE b = pixel & 0xFF;
              
              // 綠色色鍵檢測：綠色較高，紅藍較低
              if (g > 200 && r < 100 && b < 100) {
                isTransparent = true;
              }
            } else if (isPNG) {
              // PNG: 檢查 alpha 通道
              BYTE alpha = (pixel >> 24) & 0xFF;
              if (alpha < 32) { // 只有幾乎完全透明的像素才視為透明（alpha < 12.5%）
                isTransparent = true;
              }
            }
            
            int index = y * desc.Width + x;
            mask.mask[index] = !isTransparent;
          }
        }
        
        surface->UnlockRect();
      }
    }
    
    // 計算透明像素的百分比
    int transparentCount = 0;
    for (bool opaque : mask.mask) {
      if (!opaque) transparentCount++;
    }
    float transparentPercent = (float)transparentCount / (mask.width * mask.height) * 100.0f;
    
    // 存入快取
    alphaMaskCache_[imagePath] = std::move(mask);
    
    // 輸出調試信息
    char debugMsg[256];
    sprintf_s(debugMsg, "Built alpha mask for %ls: %dx%d, %.1f%% transparent pixels\n", 
              imagePath.c_str(), mask.width, mask.height, transparentPercent);
    OutputDebugStringA(debugMsg);
    
  } catch (...) {
    // 錯誤處理
  }
}

// 遞歸查找組件的輔助函數
UIComponentNew* FindComponentInTree(const std::vector<std::unique_ptr<UIComponentNew>>& components, 
                                   std::function<bool(UIComponentNew*)> predicate) {
  for (const auto& comp : components) {
    if (predicate(comp.get())) {
      return comp.get();
    }
    
    // 遞歸搜索子組件
    UIComponentNew* found = FindComponentInTree(comp->children, predicate);
    if (found) {
      return found;
    }
  }
  return nullptr;
}

UIComponentNew* UIManager::FindComponentByName(const std::wstring& name) {
  return FindComponentInTree(rootComponents_, [&name](UIComponentNew* comp) {
    return comp->name == name;
  });
}

UIComponentNew* UIManager::FindComponentById(int id) {
  return FindComponentInTree(rootComponents_, [id](UIComponentNew* comp) {
    return comp->id == id;
  });
}

// UI事件監聽器管理
void UIManager::AddUIListener(IUIListener* listener) {
  if (listener) {
    uiEventListeners_.push_back(listener);
  }
}

void UIManager::RemoveUIListener(IUIListener* listener) {
  uiEventListeners_.erase(
    std::remove(uiEventListeners_.begin(), uiEventListeners_.end(), listener),
    uiEventListeners_.end()
  );
}

void UIManager::NotifyButtonClicked(UIButtonNew* button) {
  for (auto* listener : uiEventListeners_) {
    listener->OnButtonClicked(button);
    listener->OnComponentClicked(button);
  }
}

void UIManager::NotifyComponentClicked(UIComponentNew* component) {
  for (auto* listener : uiEventListeners_) {
    listener->OnComponentClicked(component);
    
    // 也通知特定類型的事件
    if (auto* btn = dynamic_cast<UIButtonNew*>(component)) {
      listener->OnButtonClicked(btn);
    } else if (auto* img = dynamic_cast<UIImageNew*>(component)) {
      listener->OnImageClicked(img);
    }
  }
}