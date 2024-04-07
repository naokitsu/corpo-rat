#include "Startup.h"

#include <cwchar>


LPCWCHAR Startup::startup_name_ = L"corporat";

bool Startup::IsSet() {
  DWORD type = 0;
  HKEY result = nullptr;

  LSTATUS status = RegOpenKeyEx(
    HKEY_CURRENT_USER,
    LR"(Software\Microsoft\Windows\CurrentVersion\Run)",
    0,
    KEY_READ,
    &result
  );
  if (status == ERROR_SUCCESS) {
    status = RegQueryValueExW(
      result,
      startup_name_,
      nullptr,
      &type,
      nullptr,
      nullptr
    );
    return status == ERROR_SUCCESS;
  }
  return false;
}

bool Startup::Set(PCWSTR path, PCWSTR address, PCWSTR port) {
  HKEY result = nullptr;
  DWORD dwSize;

  constexpr size_t size = MAX_PATH * 2;
  wchar_t data[size] = {};
  wchar_t *cursor = data;
  wcscpy_s(cursor, size, L"\"");
  cursor += 1;
  wcscat_s(cursor, size, path);
  cursor += std::wcslen(path);
  wcscat_s(cursor, size, L"\" ");
  cursor += 2;
  wcscat_s(cursor, size, address);
  cursor += std::wcslen(address);
  wcscpy_s(cursor, size, L" ");
  cursor += 1;
  wcscat_s(cursor, size, port);

  LSTATUS status = RegCreateKeyExW(
    HKEY_CURRENT_USER,
    LR"(Software\Microsoft\Windows\CurrentVersion\Run)",
    0,
    nullptr,
    0,
    (KEY_WRITE | KEY_READ),
    nullptr,
    &result,
    nullptr
  );

  bool success = status == 0;

  if (success) {
    dwSize = (wcslen(data) + 1) * 2;
    status = RegSetValueExW(
      result,
      startup_name_,
      0,
      REG_SZ,
      reinterpret_cast<BYTE *>(data),
      dwSize
      );
    success = status == 0;
  }

  if (result != nullptr) {
    RegCloseKey(result);
    result = nullptr;
  }

  return success;
}
