#include <iostream>
#include "Client.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
  constexpr int retry_connection_ms = 3000;
  constexpr int retry_socket_ms = 3000;

  WSADATA wsa_data;

  if (WSAStartup(MAKEWORD(2, 1), &wsa_data) != 0) {
    std::cerr << "Unable to init Windows Sockets";
    return 1;
  }

  while (true) {
    try {
      Client connection;

      while (connection.Connect("127.0.0.1", 4444) != Client::Status::kOK) {
        std::cerr << "Failed to connect, retry in " << retry_connection_ms <<
          "ms\n";
        Sleep(retry_connection_ms);
      }
      
      WaitForSingleObject(connection.tcp_thread_, INFINITE);
    }
    catch (Client::Status &e) {
      std::cerr << e.what() << "\n retry in " << retry_socket_ms << "ms\n";
      Sleep(retry_socket_ms);
    }
  }
  
  WSACleanup();
  return 0;
}
