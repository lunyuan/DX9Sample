# DirectX 9 專案建置說明

## 🔧 建置環境需求

### 必要軟體
- **Visual Studio 2022** - 需要 Community 版本或更高
- **DirectX SDK (June 2010)** - 包含 d3dx9.h, d3dx9.lib
- **FBX SDK 2020.3.7** - Autodesk FBX 支援
- **Windows 10 SDK** - 系統 API 支援

### 系統需求
- Windows 10/11 x64
- DirectX 9 相容顯示卡

## 🚀 建置命令

### 推薦方法：Visual Studio IDE
```bash
# 1. 開啟 Visual Studio 2022
# 2. 檔案 → 開啟 → 專案/方案
# 3. 選擇 DX9Sample.sln
# 4. 建置 → 建置方案 (Ctrl+Shift+B)
```

### 命令列方法
```bash
# 切換到專案目錄
cd "C:\Users\james\Documents\DX9Sample"

# 使用 MSBuild 編譯 (Debug x64)
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.vcxproj -p:Configuration=Debug -p:Platform=x64

# 編譯 Release 版本
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.vcxproj -p:Configuration=Release -p:Platform=x64

# 減少輸出訊息
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.vcxproj -p:Configuration=Debug -p:Platform=x64 -v:minimal
```

### 清理專案
```bash
# 清理建置檔案
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DX9Sample.vcxproj -t:Clean -p:Configuration=Debug -p:Platform=x64
```

## 📂 輸出位置

### 建置輸出
- **可執行檔**: `test\DX9Sample.exe`
- **偵錯檔**: `test\DX9Sample.pdb`
- **中間檔**: `DX9Sample\x64\Debug\` 或 `DX9Sample\x64\Release\`

### 資源檔案位置
- **模型檔**: `test\horse_group.x`
- **材質檔**: `test\Horse2.bmp`
- **其他資源**: `test\*.bmp`, `test\*.x`

## ⚠️ 已知問題

### 連結器錯誤
目前編譯成功但連結階段出現 DirectX 函式庫未找到錯誤：
```
LNK2019: unresolved external symbol D3DXCreateBuffer
LNK2019: unresolved external symbol Direct3DCreate9
LNK2019: unresolved external symbol D3DXLoadMeshFromXW
```

### 解決方案
需要在專案設定中加入：
- d3d9.lib
- d3dx9.lib (或 d3dx9d.lib for Debug)
- 正確的 DirectX SDK include/lib 路徑

## 🔧 專案配置

### C++ 標準
- **C++20** (`/std:c++20`)

### 預處理器定義
- `_DEBUG` (Debug 版本)
- `x64`
- `_WINDOWS`
- `_UNICODE`
- `UNICODE`
- `_CRT_SECURE_NO_WARNINGS`

### 編譯器選項
- `/ZI` - 偵錯資訊
- `/JMC` - Just My Code
- `/W3` - 警告等級 3
- `/EHsc` - 例外處理
- `/MTd` - 多執行緒偵錯執行階段

## 🏃 執行程式

### 從命令列執行
```bash
cd "C:\Users\james\Documents\DX9Sample\test"
.\DX9Sample.exe
```

### 從 Visual Studio 執行
- 設定 DX9Sample 為啟始專案
- 按 F5 (偵錯執行) 或 Ctrl+F5 (不偵錯執行)

## 📊 建置狀態

### ✅ 已完成
- 編譯錯誤修正
- 介面一致性修正
- 參數類型修正
- 架構重構

### 🔄 進行中
- DirectX 函式庫連結問題
- 依賴函式庫配置

### ⏳ 待完成
- 執行測試
- 渲染功能驗證

---

**最後更新**: 2025-01-20
**編譯器**: MSVC 14.44.35207 (Visual Studio 2022)
**目標平台**: Windows x64