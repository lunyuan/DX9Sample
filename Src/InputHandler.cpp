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
    
    // Debug output for click events
    if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN) {
    }
    
    for (size_t i = 0; i < listeners_.size(); ++i) {
      if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN) {
      }
      
      if (listeners_[i]->HandleMessage(msg)) {
        if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN) {
        }
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
