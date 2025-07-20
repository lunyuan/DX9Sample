#pragma once
#include "IInputHandler.h"

class InputHandler : public IInputHandler {
public:
  InputHandler(HWND hwnd = nullptr) : hwnd_(hwnd) {}
  HRESULT ProcessMessages() override;
  void RegisterListener(IInputListener* listener) override;

private:
  std::vector<IInputListener*> listeners_;
  HWND hwnd_;
};
