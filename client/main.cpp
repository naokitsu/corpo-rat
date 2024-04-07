#include <iostream>

#include <windows.h>
// ReSharper disable once CppWrongIncludesOrder
// Windows header shall be above gdiplus.h
#include <gdiplus.h>
#include <string>
#include <comdef.h>

#include "Client.h"
#include "Startup.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib,"Gdiplus.lib")
#pragma comment(linker, "/ENTRY:wmainCRTStartup")


void exiting() {
  WSACleanup();
}

int wmain(int argc, wchar_t *argv[]) {
  if (argc != 3) {
    return 1;
  }
  
  if (!Startup::IsSet()) {
    
    wchar_t buf[MAX_PATH];
    DWORD bin_len = GetModuleFileName(0, buf, MAX_PATH);
    Startup::Set(buf, argv[1], argv[2]);
  }
  
  _bstr_t w_address(argv[1]);
  const char *address = w_address;
  unsigned short port = std::stoi(argv[2]);
  constexpr int retry_connection_ms = 3000;
  constexpr int retry_socket_ms = 3000;

  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 1), &wsa_data) != 0) {
    return 1;
  }
  (void)std::atexit(exiting);

  while (true) {
    try {
      Client connection;
      while (connection.Connect(address, port) != Client::Status::kOK) {
        Sleep(retry_connection_ms);
      }
      connection.WaitTcp();
    }
    catch (Client::Status &) {
      Sleep(retry_socket_ms);
    }
  }
}
