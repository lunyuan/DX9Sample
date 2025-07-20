// === InputHandler.cpp ===
#include "InputHandler.h"
#include <string>

// Factory 函式實作
std::unique_ptr<IInputHandler> CreateInputHandler(HWND hwnd) {
  return std::make_unique<InputHandler>(hwnd);
}

void InputHandler::RegisterListener(IInputListener* listener) {
  listeners_.push_back(listener);
}

HRESULT InputHandler::ProcessMessages() {
  MSG msg;
  
  while (PeekMessage(&msg, hwnd_, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY) {
      OutputDebugStringA("*** Exit message received ***\n");
      return S_FALSE;
    }
    
    // Let our listeners handle the message first
    bool handled = false;
    for (auto l : listeners_) {
      if (l->HandleMessage(msg)) {
        handled = true;
        break;
      }
    }
    
    // Only dispatch to default window procedure if not handled by our listeners
    if (!handled) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return S_OK;
}
