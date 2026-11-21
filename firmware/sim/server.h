// Minimal HTTP and WebSocket server for the simulator, single-threaded: the
// main loop polls its sockets next to stdin, so the core never runs on two
// threads. Serves the phone app (its build directory) at /, the simulator page
// at /sim and the protocol on /ws.
#pragma once

#include <poll.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace sim {

class Server {
 public:
  static constexpr int kMaxWebSockets = 4;
  static constexpr size_t kMaxFrame = 65536;

  std::function<void(int client)> onOpen;
  std::function<void(int client, const std::string &text)> onMessage;
  std::function<void(int client)> onClose;

  // Listens on 127.0.0.1:port (or every interface with `everywhere`).
  // webDir holds the simulator page, appDir the phone app's build.
  bool listen(int port, bool everywhere, const std::string &webDir, const std::string &appDir);
  void addPollFds(std::vector<pollfd> &fds);
  void handle(const std::vector<pollfd> &fds);  // after poll()
  void send(int client, const std::string &text);

 private:
  struct Conn {
    int fd = -1;
    int id = 0;
    bool ws = false;
    bool closing = false;
    std::string in, out;
  };
  void accept();
  void readFrom(Conn &c);
  void flush(Conn &c);
  void httpRequest(Conn &c);
  void wsFrames(Conn &c);
  void close(int fd);
  void reply(Conn &c, int code, const char *status, const std::string &type, const std::string &body);
  int wsCount() const;

  int listenFd_ = -1;
  std::string webDir_, appDir_;
  int nextId_ = 1;
  std::map<int, Conn> conns_;  // by fd
};

}  // namespace sim
