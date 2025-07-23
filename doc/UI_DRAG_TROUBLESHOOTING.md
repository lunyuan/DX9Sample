# UI 拖曳系統故障排除指南

## 已解決問題

### 1. 相機干擾問題 ✅

**問題描述**：拖曳 UI 元素時，3D 相機同時響應滑鼠移動，導致場景旋轉。

**根本原因**：UIManager 在處理拖曳開始時沒有返回 true，導致消息繼續傳播到 CameraController。

**解決方案**：
```cpp
// UIManager::HandleMessage - WM_LBUTTONDOWN
if (component->IsDraggable()) {
    // ... 開始拖曳 ...
    component->OnDragStart();
    
    // 重要：必須返回 true 以阻止相機處理
    return true;  // 修正：原本繼續執行並返回 OnMouseDown 的結果
}
```

**調試過程**：
1. 添加調試輸出追蹤消息流程
2. 發現 InputHandler 將消息傳遞給兩個 listener
3. 確認 UIManager 返回值問題
4. 修正後測試驗證

**經驗教訓**：
- 消息處理鏈中返回值的重要性
- "DEBUG FIRST" - 先調試再修改
- InputHandler 的 listener 註冊順序決定優先級

### 2. UI 序列化快取問題 ✅

**問題描述**：修改拖曳模式後，UI 元素仍使用舊的設定。

**根本原因**：UISerializer 從 ui_layout.json 載入舊的 draggable 布林值，而非新的 DragMode 枚舉。

**解決方案**：
1. 更新 UISerializer 以序列化 DragMode：
```cpp
// 保存
j["dragMode"] = static_cast<int>(component->dragMode);

// 載入
component->dragMode = static_cast<DragMode>(j.value("dragMode", 0));
```

2. 刪除舊的 ui_layout.json 檔案以強制重新生成

**預防措施**：
- 修改資料結構時同步更新序列化程式碼
- 考慮版本控制機制

### 3. 透明度檢測問題 ✅

**問題描述**：點擊透明區域仍可拖曳圖片。

**解決方案**：
```cpp
bool IsPointInTransparentArea(int x, int y, const std::wstring& imagePath, const RECT& imageRect) {
    // 實現綠色色鍵檢測 (綠色>200, 紅<100, 藍<100)
    // 使用 D3DLOCKED_RECT 讀取實際像素數據
}
```

**檢測邏輯**：
- 在 GetComponentAt() 中整合透明度檢測
- 支援 allowDragFromTransparent 標誌控制行為

### 4. 拖曳檢測順序問題 ✅

**問題描述**：子組件的拖曳事件被父容器攔截。

**解決方案**：調整 GetDraggableComponentAt 邏輯：
```cpp
// 先檢查子組件
UIComponentNew* draggableChild = findComponent(comp->children);
if (draggableChild) {
    return draggableChild;
}

// 再檢查當前組件
if (comp->IsDraggable()) {
    return comp.get();
}
```

## 已知限制

### 1. 座標系統不一致 ⚠️

**問題**：拖曳時座標變化過大，不符合 1:1 移動預期。

**表現**：
- 小幅 delta 移動導致巨大位置變化
- 子組件與父組件位置不同步

**可能原因**：
- GetAbsoluteRect() 計算可能有誤
- 座標轉換邏輯問題

**建議調試方向**：
```cpp
// 添加座標調試輸出
char debugMsg[256];
sprintf_s(debugMsg, "Drag: mouse(%d,%d) -> component(%d,%d) delta(%d,%d)\n",
          mouseX, mouseY, component->relativeX, component->relativeY, deltaX, deltaY);
OutputDebugStringA(debugMsg);
```

### 2. 功能限制

- **無拖曳預覽**：直接移動實際組件
- **無多選支援**：一次只能拖曳一個項目
- **無拖曳約束**：無法限制拖曳範圍
- **無撤銷功能**：刪除的組件無法恢復

## 調試技巧

### 1. 關鍵調試點

**消息處理流程**：
```cpp
// InputHandler.cpp
for (size_t i = 0; i < listeners_.size(); ++i) {
    if (listeners_[i]->HandleMessage(msg)) {
        // 在此處添加斷點或輸出
        handled = true;
        break;
    }
}
```

**拖曳狀態追蹤**：
```cpp
// UIManager.cpp - HandleMessage
OutputDebugStringA("Drag state: ");
OutputDebugStringA(isDragging_ ? "dragging" : "not dragging");
OutputDebugStringA(isInDragDropMode_ ? ", drop mode" : ", move mode");
```

### 2. 常見問題檢查清單

- [ ] UIManager 是否在 InputHandler 中優先註冊？
- [ ] 拖曳開始時是否返回 true？
- [ ] SetCapture/ReleaseCapture 是否配對？
- [ ] 拖曳結束時是否清理所有狀態？
- [ ] ui_layout.json 是否包含過時資料？

### 3. 性能分析

如果拖曳卡頓：
1. 檢查 GetComponentAt 的遞迴深度
2. 確認透明度檢測是否過於頻繁
3. 考慮快取絕對座標計算結果

## 最佳實踐

### 1. DEBUG FIRST 原則

根據 CLAUDE.md：
> "時間就是金錢，朋友" - 不要浪費時間修復已經修復的問題！

**實施步驟**：
1. 添加調試輸出
2. 重現問題
3. 分析輸出
4. 形成假設
5. 驗證修正

### 2. 消息處理注意事項

- **返回值很重要**：返回 true 停止消息傳播
- **處理順序**：先註冊的 listener 先處理
- **狀態一致性**：確保所有狀態變數同步更新

### 3. 代碼修改流程

1. **查看現有文檔**：檢查問題是否已記錄
2. **讀取當前代碼**：不要依賴記憶或快取
3. **添加調試輸出**：理解問題再修改
4. **最小化改動**：只修改必要部分
5. **測試驗證**：確認修正且無副作用
6. **清理調試代碼**：保持代碼整潔

## 錯誤訊息參考

### "AllocateHierarchy: XXX.BMP not found"
- **含義**：紋理檔案缺失
- **影響**：使用後備紋理，功能正常
- **解決**：確認紋理檔案存在於正確路徑

### 拖曳無反應
- **檢查**：dragMode 是否設為 None
- **檢查**：組件是否 visible
- **檢查**：GetDraggableComponentAt 邏輯

### 拖曳後位置錯誤
- **檢查**：座標系統（相對 vs 絕對）
- **檢查**：父子關係計算
- **檢查**：dragOffset 計算

## 未來改進建議

1. **拖曳預覽系統**：顯示半透明預覽而非移動實體
2. **拖曳約束**：限制可拖曳範圍
3. **動畫效果**：平滑的返回動畫
4. **多選支援**：同時拖曳多個組件
5. **拖曳歷史**：支援撤銷/重做操作