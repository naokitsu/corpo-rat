#include "Connection.h"

Connection::Connection() {
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    WSACleanup();
    throw ConnectionException(ConnectionException::kSOCKET);
  }
}

Connection::Connection(Connection &&connection) noexcept {
  socket_ = connection.socket_;
  connection.socket_ = INVALID_SOCKET;
}

Connection &Connection::operator=(Connection &&connection) noexcept {
  socket_ = connection.socket_;
  connection.socket_ = INVALID_SOCKET;
  return *this;
}

int Connection::Connect(const char *address, const unsigned short port,
                        const bool is_v6) const {
  sockaddr_in server_addr;
  server_addr.sin_family = is_v6 ? AF_INET6 : AF_INET;
  inet_pton(server_addr.sin_family, address, &server_addr.sin_addr.s_addr);

  server_addr.sin_port = htons(port);
  const int result = connect(
    socket_,
    reinterpret_cast<sockaddr *>(&server_addr),
    sizeof(server_addr)
  );

  if (result == SOCKET_ERROR) { return 1; }
  return 0;
}

Connection::~Connection() { closesocket(socket_); }

int Connection::send_msg(const char *message, const int length) const {
  const int bytes_sent = send(socket_, message, length, 0);
  if (bytes_sent == SOCKET_ERROR) { return 1; }
  return 0;
}
