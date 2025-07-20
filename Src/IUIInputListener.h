#pragma once
#include <windows.h>

/// <summary>UI 專屬的輸入監聽介面</summary>
struct IUIInputListener {
  virtual ~IUIInputListener() = default;
  /// 收到一則 UI 相關的 Win32 訊息，回傳 true 表示已處理
  virtual bool OnUIMessage(const MSG& msg) = 0;
};