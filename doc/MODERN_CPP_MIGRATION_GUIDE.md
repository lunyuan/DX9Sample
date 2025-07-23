# 現代 C++ 遷移指南

**專案**: DX9Sample  
**日期**: 2025-07-23  
**C++ 標準**: C++20

## 概述

本指南記錄了將傳統 C++ 程式碼遷移至現代 C++ 的最佳實踐和具體實作方法。

## 1. 智慧指標使用準則

### 1.1 std::unique_ptr - 獨佔所有權

**使用場景**：
- 單一擁有者的資源
- Factory 函式返回值
- 類別成員變數（擁有的資源）

**範例**：
```cpp
// Factory 函式
std::unique_ptr<IEngineContext> CreateEngineContext() {
    return std::make_unique<EngineContext>();
}

// 類別成員
class EngineContext {
private:
    std::unique_ptr<ITextureManager> textureManager_;
    std::unique_ptr<IEffectManager> effectManager_;
};

// 轉移所有權給 C API
auto frame = std::make_unique<FrameEx>();
// ... 初始化 ...
*ppNewFrame = frame.release(); // 轉移所有權
```

### 1.2 原始指標的合理使用

**保留原始指標的情況**：
1. **COM 介面** - IDirect3D*, ID3DX*
2. **非擁有參考** - 觀察者模式、父子關係
3. **C API 互操作** - DirectX 回呼函式
4. **可選參數** - nullptr 表示"無"

**範例**：
```cpp
// COM 介面 - 不使用智慧指標
IDirect3DDevice9* device_;
ID3DXMesh* mesh_;

// 非擁有參考
class UIComponent {
    UIComponent* parent_ = nullptr;  // 不擁有父元件
    UIManager* manager_ = nullptr;   // 不擁有管理器
};

// 服務定位器 - 不擁有服務
class ServiceLocator {
    IAssetManager* assetManager_;
    IUIManager* uiManager_;
};
```

### 1.3 避免的模式

```cpp
// ❌ 錯誤：手動 new/delete
MyClass* obj = new MyClass();
delete obj;

// ✅ 正確：使用智慧指標
auto obj = std::make_unique<MyClass>();

// ❌ 錯誤：使用 malloc/free
char* buffer = (char*)malloc(size);
free(buffer);

// ✅ 正確：使用 std::vector
std::vector<char> buffer(size);

// ❌ 錯誤：_strdup
char* name = _strdup(source);
free(name);

// ✅ 正確：使用 std::string
std::string name = source;
```

## 2. 字串處理

### 2.1 寬字串與窄字串轉換

**問題**：`std::codecvt_utf8` 在 C++17 中被棄用

**解決方案**：使用 Windows API
```cpp
// 寬字串轉 UTF-8
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 
                        &result[0], size, nullptr, nullptr);
    return result;
}

// UTF-8 轉寬字串
std::wstring UTF8ToWString(const std::string& str) {
    if (str.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 
                                   nullptr, 0);
    if (size <= 0) return L"";
    
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 
                        &result[0], size);
    return result;
}
```

## 3. 類型一致性

### 3.1 class vs struct

**規則**：
- 介面使用 `struct`（預設 public）
- 實作可使用 `class`（預設 private）
- 前向宣告必須匹配定義

```cpp
// 介面定義
struct IAssetManager {
    virtual ~IAssetManager() = default;
    // ...
};

// 前向宣告必須一致
struct IAssetManager;  // ✅ 正確
class IAssetManager;   // ❌ 錯誤：導致 C4099 警告

// 實作
struct AssetManager : public IAssetManager {
    // 或
class AssetManager : public IAssetManager {
public:
    // ...
};
```

### 3.2 size_t 到 int 轉換

**問題**：64位元系統上 size_t 是 8 bytes，int 是 4 bytes

**解決方案**：明確轉換
```cpp
// ❌ 隱含轉換
int index = container.size();

// ✅ 明確轉換
int index = static_cast<int>(container.size());

// ✅ 更好：使用正確的類型
size_t index = container.size();
```

## 4. 例外安全

### 4.1 RAII 原則

```cpp
// 使用智慧指標確保例外安全
auto resource = std::make_unique<Resource>();
// 如果下面的操作拋出例外，resource 會自動清理
DoSomethingThatMightThrow();
return resource.release(); // 只在成功時轉移所有權
```

### 4.2 未使用的例外參數

```cpp
// ❌ 未使用的參數產生警告
catch (const std::exception& e) {
    // 不使用 e
}

// ✅ 省略參數名稱
catch (const std::exception&) {
    // 錯誤處理
}
```

## 5. Factory Pattern

### 5.1 標準 Factory 函式

```cpp
// 標頭檔 (IMyInterface.h)
struct IMyInterface {
    virtual ~IMyInterface() = default;
    virtual void DoSomething() = 0;
};

// Factory 函式宣告
std::unique_ptr<IMyInterface> CreateMyInterface();

// 實作檔 (MyImplementation.cpp)
class MyImplementation : public IMyInterface {
public:
    void DoSomething() override { /* ... */ }
};

std::unique_ptr<IMyInterface> CreateMyInterface() {
    return std::make_unique<MyImplementation>();
}
```

### 5.2 帶參數的 Factory

```cpp
std::unique_ptr<ITextureManager> CreateTextureManager(
    ComPtr<IDirect3DDevice9> device) {
    return std::make_unique<TextureManager>(device);
}
```

## 6. 專案組織

### 6.1 目錄結構

```
DX9Sample/
├── Include/          # 公開介面
│   ├── IEngine.h
│   ├── IRenderer.h
│   └── ...
├── Src/             # 內部實作
│   ├── Engine.cpp
│   ├── Engine.h
│   └── ...
├── test/            # 輸出目錄和資源
│   ├── *.exe
│   ├── *.bmp
│   └── *.x
└── doc/             # 文件
```

### 6.2 Include 路徑

```cpp
// 從 Src 目錄引用 Include
#include "../Include/IEngine.h"

// 從根目錄引用 Include
#include "Include/IEngine.h"

// 從根目錄引用 Src
#include "Src/Engine.h"
```

## 7. C++20 新功能

### 7.1 可用功能

- **Concepts** - 模板約束
- **Ranges** - 更好的演算法
- **Coroutines** - 非同步程式設計
- **Three-way comparison** - `<=>`
- **Designated initializers** - `{.x = 1, .y = 2}`
- **std::format** - 類型安全的格式化

### 7.2 範例

```cpp
// Designated initializers
D3DLIGHT9 light{
    .Type = D3DLIGHT_DIRECTIONAL,
    .Diffuse = {1.0f, 1.0f, 1.0f, 1.0f},
    .Direction = {0.0f, -1.0f, 0.0f}
};

// Concepts (未來可用)
template<typename T>
requires std::derived_from<T, IComponent>
void RegisterComponent(std::unique_ptr<T> component);
```

## 結論

遵循這些準則可以確保程式碼：
- 記憶體安全（無洩漏）
- 例外安全（RAII）
- 型別安全（明確轉換）
- 可維護性（清晰的所有權）
- 現代化（使用最新標準）

持續更新和改進是保持程式碼品質的關鍵。