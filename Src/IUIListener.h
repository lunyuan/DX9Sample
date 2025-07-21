#pragma once

// Forward declarations
struct UIComponentNew;
struct UIButtonNew;
struct UIImageNew;
struct UIEditNew;

/// <summary>
/// UI事件監聽器介面
/// 實作此介面以接收UI元件的事件通知
/// </summary>
struct IUIListener {
    virtual ~IUIListener() = default;
    
    // 按鈕點擊事件
    virtual void OnButtonClicked(UIButtonNew* button) {}
    
    // 圖片點擊事件
    virtual void OnImageClicked(UIImageNew* image) {}
    
    // 編輯框事件
    virtual void OnEditTextChanged(UIEditNew* edit) {}
    virtual void OnEditGotFocus(UIEditNew* edit) {}
    virtual void OnEditLostFocus(UIEditNew* edit) {}
    
    // 通用的元件事件
    virtual void OnComponentClicked(UIComponentNew* component) {}
    virtual void OnComponentDragStart(UIComponentNew* component) {}
    virtual void OnComponentDragEnd(UIComponentNew* component) {}
    virtual void OnComponentDragging(UIComponentNew* component, int deltaX, int deltaY) {}
    
    // 滑鼠移入/移出事件
    virtual void OnComponentMouseEnter(UIComponentNew* component) {}
    virtual void OnComponentMouseLeave(UIComponentNew* component) {}
};