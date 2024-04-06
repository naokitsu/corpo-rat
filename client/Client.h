#pragma once
#include <exception>
#include <fstream>
#include <iostream>
#include <winsock.h>

class Command {
public:
  enum Type {
    kINFO,
    kSCREEN,
    kERR
  };

private:
  Type type_;

public:
  Command(const Type type)
    : type_(type) {}

  Type type() const { return type_; }

  operator Type() const { return type_; }
};

class Client {
  SOCKET socket_;
  bool valid_;
  HANDLE tcp_thread_;
  DWORD thread_id_;

public:
  class Status {
  public:
    enum Type {
      kOK          = 0,
      kSOCKET_ERR  = 1,
      kCONNECT_ERR = 2
    };

  private:
    Type type_;

  public:
    Status(const Type type)
      : type_(type) {}

    Type type() const { return type_; }

    operator Type() const { return type_; }

    const char *what() const {
      switch (type_) {
      case kOK: { return "Ok"; }
      case kSOCKET_ERR: { return "Failed to initialize socket"; }
      case kCONNECT_ERR: { return "Failed to connect"; }
      default:
        return "Unknown error"; // Unreachable
      }
    }
  };

  Client()
    : valid_(false)
    , socket_(INVALID_SOCKET) {};
  Client(const Client &connection) = delete;

  Client(Client &&connection) noexcept;

  Client &operator=(const Client &connection) = delete;
  Client &operator=(Client &&connection) noexcept;

  Client::Status Connect(const char *address);

  void Wait() const;

  ~Client();


  bool SendMessage(const char *message, int length) const;
  Command ReceiveCommand() const;
};

DWORD Thread(const LPVOID param);

