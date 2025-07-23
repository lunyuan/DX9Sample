# Model Loading Debug Log

## 載入流程分析

### 1. X 文件載入 (test1.x)
```
檔案: test1.x
模型數量: 7 匹馬
骨骼數量: 8 個 joints
```

### 2. 貼圖載入記錄
```
AllocateHierarchy: Attempting to load texture: HORSE3.BMP ✓
AllocateHierarchy: Attempting to load texture: RED.BMP ✗
→ 使用 Horse4.bmp 作為替代
```

### 3. 材質處理
每個馬模型包含 2 個材質：
- Material[0]: HORSE3.BMP
- Material[1]: RED.BMP (替換為 Horse4.bmp)

### 4. UV 座標驗證
```
Vertex 0: UV(0.440, 0.583) ✓
Vertex 1: UV(0.596, 0.581) ✓
Vertex 2: UV(0.446, 0.279) ✓
Vertex 3: UV(0.551, 0.321) ✓
Vertex 4: UV(0.687, 0.305) ✓
```
UV 座標正常，在 0-1 範圍內。

### 5. 模型貼圖控制
```cpp
ModelData::useOriginalTextures = false  // 預設不使用模型內的貼圖
```
載入後清除所有材質貼圖，使用 SetTexture 指定的 Horse4.bmp。

### 6. 渲染管線選擇
```
固定管線: 貼圖正常顯示 ✓
簡單 Shader: 貼圖正常顯示 ✓
骨骼動畫 Shader: 全黑 ✗
```

## Debug Output 重要訊息

### 成功訊息
- `Successfully loaded skeletal animation shader`
- `Successfully loaded simple texture shader`
- `SetTexture 成功載入貼圖: Horse4.bmp`
- `Using simple texture shader`
- `DrawWithEffect: Texture set successfully`

### 骨骼資訊
- `Using fixed pipeline: joints=8`
- 模型有骨骼但可能沒有正確的權重數據

### 頂點資訊
- 每個馬模型: 349 個頂點, 1758 個索引
- 材質數量: 2
- 貼圖指標正確設置

## 問題診斷總結

1. **根本原因**: X 文件包含骨骼定義但沒有骨骼權重數據
2. **解決方案**: 使用簡單 shader 取代骨骼動畫 shader
3. **結果**: 貼圖正常顯示，模型渲染正確

## 效能考量

- 使用 vertex declaration 而非 FVF
- DrawIndexedPrimitive 而非 DrawIndexedPrimitiveUP
- 貼圖快取避免重複載入