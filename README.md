# Corpo-rat

Corporat is an application for monitoring work activity. It silently launches on user logon and work in the background tracking activity by monitoring mouse and keyboard events and allowing server screenshot the client's desktop.

## Usage

### Client
Client can be installed and launched using
  ```
  client <ip> <port>
  ```
After that it'll add itself into the autostart and silently boot with the user logon 

### Server
Server can be launched using
  ```
  ./server <ip> <port>
  ```
`<ip>` and `<port>` are optinal, if server isn't provided any or couldn't parse the values, it is going to use default values of `0.0.0.0` and `4444` respectively

Webui can be accessed at the port `8000`
