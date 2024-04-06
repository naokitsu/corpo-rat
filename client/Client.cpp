#include "Client.h"
#include <windows.h>

#include "ScreenCapture.h"

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
  // server_addr.sin_family, address, &server_addr.sin_addr.s_addr
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

  tcp_thread_ = CreateThread(
    NULL,
    0,
    Thread,
    (LPVOID)this,
    0,
    &thread_id_
  );
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


bool Client::SendMessage(const char *message, const int length) const {
  const int bytes_sent = send(socket_, message, length, 0);
  if (bytes_sent == SOCKET_ERROR) { return false; }
  return true;
}

Command Client::ReceiveCommand() const {
  char data;
  recv(socket_, &data, 1, 0);
  switch (data) {
  case 1:
    return Command::kTEST;
  case 2:
    return Command::kSCREEN;
  }
  return Command::kERR;
}



DWORD Thread(const LPVOID param) {
  const Client *client = static_cast<Client *>(param);
  while (true) {
    Command command = client->ReceiveCommand();
    if (command == Command::kERR) {
      break;
    }
    switch (command) {
    case Command::kTEST: {
      while (!client->SendMessage("Pong", 4)) {};
      break;
    }
    case Command::kSCREEN: {
      IStream *file = Screenshot();
      STATSTG stats;
      file->Stat(&stats, 0);
      auto size = stats.cbSize.QuadPart;
      std::cout << std::hex << size;
      while (!client->SendMessage(reinterpret_cast<char *>(&size), 8)) {};
      constexpr int chunk_size = 1024;
      char *data = new char[chunk_size];
      for (ULONGLONG i = 0; i < size;) {
        ULONG read_bytes;
        file->Read(data, chunk_size, &read_bytes);
        while (!client->SendMessage(data, read_bytes)) {};
        i += read_bytes;
        std::cout << '.' << read_bytes << '\n';
      }
      std::cout << "end";
      break;
    }
    default:
      break;
    }}
  return 0;
}