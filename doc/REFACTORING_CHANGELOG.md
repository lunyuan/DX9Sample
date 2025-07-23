# 重構變更記錄

**專案**: DX9Sample  
**日期**: 2025-07-23  
**版本**: 2.0.0

## 變更清單

### 🏗️ 架構重構

#### 檔案重組
- [x] 將所有介面檔案 (I*.h) 從 Src/ 移至 Include/
- [x] 將實作類別標頭檔從 Include/ 移至 Src/
  - DirectionalLight.h
  - EventManager.h  
  - EngineContext.h
  - SkinMesh.h
  - Skeleton.h
  - XFileTypes.h
  - Utilities.h
  - UICoordinateFix.h
  - ModelData.h
- [x] 將應用層級檔案移至專案根目錄
  - main.cpp
  - Scene 相關檔案 (GameScene, PauseScene, SettingsScene, Scene3D)

#### Include 路徑更新
- [x] 更新 DX9Sample.vcxproj 加入 Include 目錄
- [x] 修正所有 #include 路徑以反映新結構
- [x] 使用相對路徑 (../Include/, ../Src/) 確保正確引用

### 🏭 Factory Pattern

#### 已驗證的 Factory 函式
- [x] CreateAssetManager()
- [x] CreateCameraController() 
- [x] CreateConfigManager()
- [x] CreateD3DContext()
- [x] CreateEffectManager()
- [x] CreateEngineContext()
- [x] CreateEventManager()
- [x] CreateFullScreenQuad()
- [x] CreateInputHandler()
- [x] CreateLightManager()
- [x] CreateModelManager()
- [x] CreateFbxSaver()
- [x] CreateScene3D()
- [x] CreateSceneManager()
- [x] CreateTextureManager()
- [x] CreateUIManager()

### 🔧 現代 C++ 改進

#### 智慧指標重構
- [x] AllocateHierarchy.cpp - 使用 std::make_unique
- [x] 移除手動 new/delete
- [x] 使用 RAII 進行資源管理
- [x] 保留必要的原始指標（COM 介面、非擁有參考）

#### 字串處理
- [x] 移除 _strdup/free
- [x] 替換過時的 std::codecvt_utf8
- [x] 實作 WStringToUTF8() 和 UTF8ToWString() 使用 Windows API

### 🐛 編譯警告修正

#### C4099 - class/struct 不一致
- [x] ServiceLocator: class → struct
- [x] IAssetManager 前向宣告修正
- [x] IServiceLocator 前向宣告加入並統一為 struct

#### C4101 - 未使用的變數
- [x] GltfModelLoader.cpp - catch 區塊
- [x] SimpleGltfConverter.cpp - catch 區塊
- [x] MultiModelGltfConverter.cpp - catch 區塊

#### C4267 - size_t 到 int 轉換
- [x] MultiModelGltfConverter.cpp - 加入 static_cast<int>()

#### C4477 - sprintf_s 格式字串
- [x] FbxLoader.cpp - 修正 FbxString 參數

#### C4996 - 過時的 API
- [x] UISerializer.cpp - 替換 codecvt 為 Windows API

### 📋 專案設定

- [x] 確認使用 C++20 標準
- [x] 設定 AdditionalIncludeDirectories
- [x] 維持 x64 Debug 組態

### 📊 最終建置狀態

```
警告: 0
錯誤: 0
C++ 標準: C++20
平台: x64
組態: Debug
```

## 重要決策記錄

### 1. 為何保留某些原始指標

**COM 介面**：DirectX 使用 COM，必須使用原始指標
```cpp
IDirect3DDevice9* device_;  // COM 介面
```

**非擁有參考**：ServiceLocator 不擁有服務
```cpp
class ServiceLocator {
    IAssetManager* assetManager_;  // 不擁有，只是註冊表
};
```

### 2. 字串轉換方法選擇

選擇 Windows API 而非 std::codecvt：
- std::codecvt 在 C++17 中被棄用
- Windows API 更有效率且穩定
- 專案已經依賴 Windows 平台

### 3. 檔案組織原則

- **Include/**: 只包含純介面定義
- **Src/**: 包含所有實作和內部類別
- **根目錄**: 應用層級程式碼（場景、主程式）

## 未來改進建議

1. **單元測試**: 利用介面分離優勢加入測試
2. **文件註解**: 為所有公開 API 加入 Doxygen 註解
3. **命名空間**: 考慮加入命名空間組織程式碼
4. **CMake 支援**: 加入跨平台建置支援

## 相關文件

- [PROJECT_REFACTORING_SUMMARY.md](PROJECT_REFACTORING_SUMMARY.md) - 重構總結
- [MODERN_CPP_MIGRATION_GUIDE.md](MODERN_CPP_MIGRATION_GUIDE.md) - 現代 C++ 指南
- [ARCHITECTURE_ANALYSIS.md](ARCHITECTURE_ANALYSIS.md) - 架構分析
- [README.md](README.md) - 專案總覽