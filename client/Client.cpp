#include "Client.h"

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
  valid_ = true;
  
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
  closesocket(socket_);
  return Status::kOk;
}


bool Client::send_msg(const char *message, const int length) const {
  const int bytes_sent = send(socket_, message, length, 0);
  if (bytes_sent == SOCKET_ERROR) { return false; }
  return true;
}
