#include "Client.h"
#include <windows.h>

Client::Client(Client &&connection) noexcept {
  socket_ = connection.socket_;
  valid_ = connection.valid_;
  connection.socket_ = INVALID_SOCKET;
  connection.valid_ = false;
}

Client::Status Client::Connect(const char *address, const unsigned short port,
                        const bool is_v6) {
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    valid_ = false;
    return Status::kSOCKET_ERR;
  }
  
  sockaddr_in server_addr;
  server_addr.sin_family = is_v6 ? AF_INET6 : AF_INET;
  inet_pton(server_addr.sin_family, address, &server_addr.sin_addr.s_addr);

  server_addr.sin_port = htons(port);
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

Client::~Client() {
// TODO kill thread
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
  return Command::kTEST;
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
      while (!client->SendMessage("Pong", 4));
      break;
    }
    default:
      break;
    }}
  return 0;
}