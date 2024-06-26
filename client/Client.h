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
    kACTIVITY,
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
  HANDLE activity_tracker_;
  HANDLE pump_thread_;
  HHOOK mouse_hook_;
  HHOOK keyboard_hook_;
  HANDLE stop_events_[3];

  static DWORD thread_id_;
  static std::atomic<unsigned int> inactive_for_;

public:
  class Status {
  public:
    enum Type {
      kOK          = 0,
      kSOCKET_ERR  = 1,
      kCONNECT_ERR = 3,
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

  Client();

  Client(const Client &connection) = delete;

  Client(Client &&connection) noexcept;

  Client &operator=(const Client &connection) = delete;
  Client &operator=(Client &&connection) noexcept;

  Status Connect(const char *address, unsigned short port);

  void WaitTcp() const;

  ~Client();


  bool Send(const char *message, int length) const;
  Command Receive() const;

  static DWORD TcpThread(LPVOID param);
  static DWORD ActivityThread(LPVOID param);
  static DWORD PumpThread(LPVOID param);
  static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam,
                                           LPARAM lParam);
  static LRESULT CALLBACK
  MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
};
