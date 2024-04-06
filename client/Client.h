#pragma once
#include <exception>
#include <winsock2.h>
#include <WS2tcpip.h>

class Client {
  SOCKET socket_;
  bool valid_;
  HANDLE tcp_thread_;

public:
  class Status {
  public:
    enum Type {
      kOk = 0,
      kSOCKET_ERR = 1,
      kCONNECT_ERR = 2
    };

  private:
    Type type_;

  public:
    Status(const Type type)
      : type_(type) {}

    Type type() const { return type_; }

    operator Type() const {
      return type_;
    }

    const char *what() const {
      switch (type_) {
      case kOk: { return "Ok"; }
      case kSOCKET_ERR: { return "Failed to initialize socket"; }
      case kCONNECT_ERR: { return "Failed to connect"; }
      default:
        return "Unknown error"; // Unreachable
      }
    }
  };
  
  Client();
  Client(const Client &connection) = delete;

  Client(Client &&connection) noexcept;

  Client &operator=(const Client &connection) = delete;
  Client &operator=(Client &&connection) noexcept;

  Client::Status Connect(const char *address, unsigned short port,
              bool is_v6 = false);

  ~Client();

  bool send_msg(const char *message, int length) const;
};
