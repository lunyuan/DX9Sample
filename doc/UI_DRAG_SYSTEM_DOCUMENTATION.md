# UI 拖曳系統文檔

## 概述

UI 拖曳系統提供了靈活的拖曳功能，支援多種拖曳模式：
- **Move**: 可移動位置（拖曳後停留在新位置）
- **MoveRevert**: 可移動但放開後回到原位
- **DragDrop**: 拖放模式（可拖曳到其他組件）

## 架構設計

### 拖曳模式枚舉

```cpp
enum class DragMode {
    None,           // 不可拖曳
    Move,           // 可移動位置（拖曳後停留在新位置）
    MoveRevert,     // 可移動但放開後回到原位
    DragDrop        // 拖放模式（可拖曳到其他組件）
};
```

### 組件介面擴展 (UIComponentNew)

```cpp
class UIComponentNew {
public:
    // 拖曳屬性
    DragMode dragMode = DragMode::None;
    int originalX = 0;
    int originalY = 0;
    
    // 拖曳相關方法
    virtual bool IsDraggable() const { return dragMode != DragMode::None; }
    virtual bool CanReceiveDrop() const { return false; }
    virtual void OnDragEnter(UIComponentNew* dragged) {}
    virtual void OnDragLeave(UIComponentNew* dragged) {}
    virtual bool OnDrop(UIComponentNew* dragged) { return false; }
    virtual void OnDragStart() {}
    virtual void OnDragEnd(bool accepted) {}
};
```

### UIManager 狀態變數

```cpp
UIComponentNew* draggedComponent_ = nullptr;  // 當前拖曳的組件
UIComponentNew* dropTarget_ = nullptr;        // 當前放置目標
bool isInDragDropMode_ = false;              // 拖放模式標誌
POINT dragOffset_ = {0, 0};                   // 滑鼠偏移
bool isDragging_ = false;                     // 拖曳狀態
```

## 實作細節

### 1. 開始拖曳 (WM_LBUTTONDOWN)

```cpp
if (component->IsDraggable()) {
    // 保存原始位置
    component->originalX = component->relativeX;
    component->originalY = component->relativeY;
    
    // 設置拖曳狀態
    draggedComponent_ = component;
    RECT rect = component->GetAbsoluteRect();
    dragOffset_.x = mouseX - rect.left;
    dragOffset_.y = mouseY - rect.top;
    
    SetCapture(msg.hwnd);
    isDragging_ = true;
    isInDragDropMode_ = (component->dragMode == DragMode::DragDrop);
    
    component->OnDragStart();
    
    // 重要：必須返回 true 以阻止其他系統（如相機）處理此事件
    return true;
}
```

### 2. 拖曳中 (WM_MOUSEMOVE)

```cpp
if (draggedComponent_ && isDragging_) {
    // 更新位置
    int newAbsX = mouseX - dragOffset_.x;
    int newAbsY = mouseY - dragOffset_.y;
    
    if (draggedComponent_->parent) {
        RECT parentRect = draggedComponent_->parent->GetAbsoluteRect();
        draggedComponent_->relativeX = newAbsX - parentRect.left;
        draggedComponent_->relativeY = newAbsY - parentRect.top;
    } else {
        draggedComponent_->relativeX = newAbsX;
        draggedComponent_->relativeY = newAbsY;
    }
    
    // DragDrop 模式下檢查放置目標
    if (isInDragDropMode_) {
        UIComponentNew* targetComponent = GetComponentAt(mouseX, mouseY);
        
        if (targetComponent != dropTarget_) {
            if (dropTarget_ && dropTarget_->CanReceiveDrop()) {
                dropTarget_->OnDragLeave(draggedComponent_);
            }
            
            dropTarget_ = targetComponent;
            
            if (dropTarget_ && dropTarget_->CanReceiveDrop()) {
                dropTarget_->OnDragEnter(draggedComponent_);
            }
        }
    }
    
    return true; // 攔截所有滑鼠移動事件
}
```

### 3. 結束拖曳 (WM_LBUTTONUP)

```cpp
if (draggedComponent_ && isDragging_) {
    bool accepted = false;
    
    // 根據拖曳模式處理
    switch (draggedComponent_->dragMode) {
        case DragMode::Move:
            // 停留在新位置
            draggedComponent_->OnDragEnd(false);
            break;
            
        case DragMode::MoveRevert:
            // 返回原始位置
            draggedComponent_->relativeX = draggedComponent_->originalX;
            draggedComponent_->relativeY = draggedComponent_->originalY;
            draggedComponent_->OnDragEnd(false);
            break;
            
        case DragMode::DragDrop:
            // 嘗試放置
            if (isInDragDropMode_ && dropTarget_ && dropTarget_->CanReceiveDrop()) {
                accepted = dropTarget_->OnDrop(draggedComponent_);
            }
            
            if (accepted) {
                // 刪除拖曳的組件
                draggedComponent_->OnDragEnd(true);
                RemoveComponent(draggedComponent_);
            } else {
                // 返回原始位置
                draggedComponent_->relativeX = draggedComponent_->originalX;
                draggedComponent_->relativeY = draggedComponent_->originalY;
                draggedComponent_->OnDragEnd(false);
            }
            break;
    }
    
    // 清理狀態
    draggedComponent_ = nullptr;
    dropTarget_ = nullptr;
    isDragging_ = false;
    isInDragDropMode_ = false;
    ReleaseCapture();
    
    return true;
}
```

## 使用範例

### 創建可拖曳的圖片

```cpp
// 創建可移動的背景圖片
auto* bgImage = uiManager->CreateImage(L"bg.png", 100, 100, 
                                      bgWidth, bgHeight, 
                                      DragMode::Move, nullptr, true);

// 創建可拖放的圖標
auto* iconImage = uiManager->CreateImage(L"icon.png", 50, 50,
                                        32, 32,
                                        DragMode::DragDrop, parentContainer);

// 創建預覽用圖片（拖曳後回復原位）
auto* previewImage = uiManager->CreateImage(L"preview.png", 200, 200,
                                           100, 100,
                                           DragMode::MoveRevert, nullptr);
```

### 設置放置目標

```cpp
// 擴展 UIImageNew 以接受放置
class DropTargetImage : public UIImageNew {
public:
    bool CanReceiveDrop() const override { return true; }
    
    void OnDragEnter(UIComponentNew* dragged) override {
        // 顯示高亮效果
        this->color = D3DCOLOR_ARGB(255, 255, 255, 128);
    }
    
    void OnDragLeave(UIComponentNew* dragged) override {
        // 恢復正常顏色
        this->color = D3DCOLOR_ARGB(255, 255, 255, 255);
    }
    
    bool OnDrop(UIComponentNew* dragged) override {
        // 處理放置邏輯
        if (ValidateDropItem(dragged)) {
            ProcessDroppedItem(dragged);
            return true;  // 接受放置
        }
        return false;  // 拒絕放置
    }
};
```

## 座標系統

系統使用相對座標系統：
- 根組件使用螢幕座標
- 子組件使用相對於父組件的座標
- `GetAbsoluteRect()` 遞迴計算絕對位置

## 視覺回饋

### 拖曳中的視覺效果
- 組件跟隨滑鼠移動
- 保持原始外觀

### 放置目標高亮
```cpp
void UIImageNew::Render(...) {
    D3DCOLOR finalColor = color;
    if (manager && static_cast<UIManager*>(manager)->GetDropTarget() == this) {
        finalColor = D3DCOLOR_ARGB(255, 255, 255, 128); // 黃色高亮
    }
}
```

## 進階功能

### 透明度檢測
支援像素級透明度檢測，防止從透明區域開始拖曳：

```cpp
if (auto* img = dynamic_cast<UIImageNew*>(comp.get())) {
    if (!img->allowDragFromTransparent && img->useTransparency && 
        IsPointInTransparentArea(x, y, img->imagePath, rect)) {
        // 跳過透明區域
        continue;
    }
}
```

### 拖曳優先級
- 子組件優先於父組件
- 後添加的組件優先於先添加的組件（z-order）

## 限制與注意事項

1. **無拖曳預覽** - 實際組件移動，而非顯示預覽圖
2. **無多選支援** - 一次只能拖曳一個組件
3. **無拖曳約束** - 組件可以拖曳到螢幕任何位置
4. **消息處理順序** - UIManager 必須在 InputHandler 中優先於其他系統註冊

## 序列化支援

拖曳設定可透過 UISerializer 保存和載入：

```cpp
// 保存
j["dragMode"] = static_cast<int>(component->dragMode);

// 載入
component->dragMode = static_cast<DragMode>(j.value("dragMode", 0));
```