#pragma once
#include <windows.h>

struct IInputListener {
  virtual ~IInputListener() = default;
  virtual bool HandleMessage(const MSG& msg) = 0;
};