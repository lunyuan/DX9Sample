#pragma once
#include <windows.h>
#include "IInputListener.h"
#include <vector>
#include <memory>

struct IInputHandler {
  virtual ~IInputHandler() {}
  /// 處理佇列中所有 Win32 訊息，並分派給 Listener
  virtual HRESULT ProcessMessages() = 0;
  /// 註冊 Listener
  virtual void RegisterListener(IInputListener* listener) = 0;
};

/// <summary>Factory 函式：建立預設實作的 InputHandler。</summary>
std::unique_ptr<IInputHandler> CreateInputHandler(HWND hwnd);