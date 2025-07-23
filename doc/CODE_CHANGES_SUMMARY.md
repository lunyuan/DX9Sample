# Code Changes Summary

## 主要修改的檔案

### 1. GameScene.cpp / GameScene.h
**修改內容**：
- 新增 `simpleTextureEffect_` 成員變數
- 載入 simple_texture.fx shader
- 修改渲染邏輯，根據條件選擇不同的渲染方式
- 調整初始模型載入，移除縮放和位置調整

**關鍵程式碼**：
```cpp
// 使用適合的shader
bool hasBoneWeights = false;
bool useSkeletalAnimation = false; // 暫時禁用骨骼動畫
bool useSimpleShader = true; // 使用簡單shader
```

### 2. SkinMesh.cpp / SkinMesh.h
**修改內容**：
- 新增 `DrawWithEffect()` 方法
- 改進 `DrawWithAnimation()` 的調試輸出
- 修正 `SetTexture()` 更新所有材質的邏輯
- 添加詳細的調試訊息輸出

**新增方法**：
```cpp
void DrawWithEffect(IDirect3DDevice9* dev, ID3DXEffect* effect);
```

### 3. XModelEnhanced.cpp
**修改內容**：
- 實作 `useOriginalTextures` 旗標邏輯
- 清除模型貼圖當 `useOriginalTextures = false`
- 添加 skinInfo 調試輸出
- 嘗試實作骨骼權重載入（但 test1.x 沒有權重數據）

**關鍵修改**：
```cpp
if (!modelData->useOriginalTextures) {
    // 清除所有材質貼圖但保留材質數據
    for (auto& material : modelData->mesh.materials) {
        if (material.tex) {
            material.tex->Release();
            material.tex = nullptr;
        }
    }
}
```

### 4. CameraController.cpp / CameraController.h
**修改內容**：
- 初始距離從 50.0f 改為 20.0f
- 最小距離從 2.0f 改為 5.0f
- 最大距離從 100.0f 改為 200.0f

### 5. AllocateHierarchy.cpp
**修改內容**：
- RED.BMP 找不到時自動使用 Horse4.bmp
- 修正材質數據遺失問題（設置基類 pMaterials 指標）

**關鍵修正**：
```cpp
// 設定基類的 pMaterials 指標
mc->pMaterials = new D3DXMATERIAL[numMaterials];
```

## 新增的檔案

### 1. simple_texture.fx
簡單的貼圖渲染 shader，支援基本光照和貼圖。

### 2. debug_texture.fx
調試用 shader（已創建但未使用）。

### 3. test_texture.fx
測試用 shader（已創建但未使用）。

## 移除的程式碼

### 1. 調試用的渲染模式切換
移除了導致閃爍的 wireframe/solid 模式切換代碼。

### 2. 硬編碼的模型數量
移除了硬編碼的 7 個模型限制，改為動態載入所有找到的模型。

## 重要的 Bug 修復

1. **材質數據遺失**：修正 AllocateHierarchy 中基類指標未設置的問題
2. **貼圖不顯示**：實作 SetTexture 更新所有材質的邏輯
3. **模型閃爍**：移除調試用的渲染模式切換
4. **骨骼動畫 shader 全黑**：暫時使用簡單 shader 替代

## 配置變更

### ModelData.h
新增控制旗標：
```cpp
bool useOriginalTextures = false;  // 預設不使用模型檔案中的貼圖
```

## 效能優化

1. 使用 vertex declaration 取代 FVF
2. 使用 DrawIndexedPrimitive 取代 DrawIndexedPrimitiveUP
3. 貼圖快取避免重複載入