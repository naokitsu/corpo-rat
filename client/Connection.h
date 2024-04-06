#pragma once
#include <winsock2.h>
#include <exception>
#include <WS2tcpip.h>

class ConnectionException : public std::exception { // TODO Derive exception
public:
  enum ExceptionType {
    kSOCKET,
    kCONNECT
  };
private:
  ExceptionType type_;
public:
  explicit ConnectionException(ExceptionType type)
    : type_(type) {}
  ExceptionType type() const { return type_; }

  const char *what() const override {
    switch (type_) {
    case kSOCKET: {
      return "Failed to initialize socket";
    }
    case kCONNECT: {
      return "Failed to connect";
    }
    default:
      return "Unknown error"; // Unreachable
    }
  }
};

class Connection {
  SOCKET socket_;

public:
  Connection();
  Connection(const Connection &connection) = delete;
  
  Connection(Connection &&connection) noexcept;

  Connection &operator=(const Connection &connection) = delete;
  Connection &operator=(Connection &&connection) noexcept;
  
  int Connect(const char *address, const unsigned short port, const bool is_v6 = false) const;

  ~Connection();

  int send_msg(const char *message, const int length) const;
};
