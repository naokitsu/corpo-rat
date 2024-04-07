#pragma once

#include <windows.h>

class Startup {
  static LPCWCHAR startup_name_;

public:
  static bool IsSet();
  static bool Set(PCWSTR path, PCWSTR address, PCWSTR port);
};
