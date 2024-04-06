#include <iostream>
#include "Connection.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
  const int retry_connection_ms = 3000;
  const int retry_socket_ms = 3000;
  
  WSADATA wsa_data;

  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    std::cerr << "Unable to init Windows Sockets";
    return 1;
  }

  while (true) {
    try {
      Connection connection;

      while (connection.Connect("127.0.0.1", 4444) != 0) {
        std::cerr << "Failed to connect, retry in " << retry_connection_ms << "ms\n";
        Sleep(retry_connection_ms);        
      }

      while (true) {
        if (connection.send_msg("Hello", 5) != 0) {
          break;
        }
        Sleep(1000);
      }
      
    } catch (ConnectionException &e) {
      std::cerr << e.what() << "\n retry in " << retry_socket_ms << "ms\n";
      Sleep(retry_socket_ms);
    }
  }
  WSACleanup();
  return 0;
}
