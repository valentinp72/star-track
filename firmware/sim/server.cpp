#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

namespace sim {

namespace {

// SHA-1 (FIPS 180-1), only for the WebSocket handshake.
std::string sha1(const std::string &msg) {
  uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  std::string m = msg;
  uint64_t bits = (uint64_t)msg.size() * 8;
  m += (char)0x80;
  while (m.size() % 64 != 56) m += (char)0;
  for (int i = 7; i >= 0; i--) m += (char)(bits >> (i * 8));
  auto rol = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
  for (size_t off = 0; off < m.size(); off += 64) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
      w[i] = (uint8_t)m[off + 4 * i] << 24 | (uint8_t)m[off + 4 * i + 1] << 16 | (uint8_t)m[off + 4 * i + 2] << 8 |
             (uint8_t)m[off + 4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
      uint32_t f, k;
      if (i < 20) f = (b & c) | (~b & d), k = 0x5A827999;
      else if (i < 40) f = b ^ c ^ d, k = 0x6ED9EBA1;
      else if (i < 60) f = (b & c) | (b & d) | (c & d), k = 0x8F1BBCDC;
      else f = b ^ c ^ d, k = 0xCA62C1D6;
      uint32_t t = rol(a, 5) + f + e + k + w[i];
      e = d, d = c, c = rol(b, 30), b = a, a = t;
    }
    h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e;
  }
  std::string out;
  for (uint32_t v : h)
    for (int i = 3; i >= 0; i--) out += (char)(v >> (i * 8));
  return out;
}

std::string base64(const std::string &in) {
  static const char *t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    uint32_t v = (uint8_t)in[i] << 16 | (uint8_t)in[i + 1] << 8 | (uint8_t)in[i + 2];
    out += t[v >> 18], out += t[(v >> 12) & 63], out += t[(v >> 6) & 63], out += t[v & 63];
  }
  if (i + 1 == in.size()) {
    uint32_t v = (uint8_t)in[i] << 16;
    out += t[v >> 18], out += t[(v >> 12) & 63], out += "==";
  } else if (i + 2 == in.size()) {
    uint32_t v = (uint8_t)in[i] << 16 | (uint8_t)in[i + 1] << 8;
    out += t[v >> 18], out += t[(v >> 12) & 63], out += t[(v >> 6) & 63], out += '=';
  }
  return out;
}

std::string header(const std::string &req, const char *name) {
  std::istringstream s(req);
  std::string line;
  size_t n = strlen(name);
  while (std::getline(s, line)) {
    if (line.size() > n && strncasecmp(line.c_str(), name, n) == 0 && line[n] == ':') {
      std::string v = line.substr(n + 1);
      while (!v.empty() && (v.front() == ' ')) v.erase(0, 1);
      while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();
      return v;
    }
  }
  return "";
}

std::string contentType(const std::string &path) {
  auto ends = [&](const char *ext) {
    size_t n = strlen(ext);
    return path.size() >= n && path.compare(path.size() - n, n, ext) == 0;
  };
  if (ends(".html")) return "text/html; charset=utf-8";
  if (ends(".js")) return "text/javascript";
  if (ends(".css")) return "text/css";
  if (ends(".json")) return "application/json";
  if (ends(".svg")) return "image/svg+xml";
  if (ends(".png")) return "image/png";
  if (ends(".webmanifest")) return "application/manifest+json";
  if (ends(".woff2")) return "font/woff2";
  if (ends(".glb")) return "model/gltf-binary";
  return "application/octet-stream";
}

}  // namespace

bool Server::listen(int port, bool everywhere, const std::string &webDir, const std::string &appDir) {
  webDir_ = webDir;
  appDir_ = appDir;
  listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd_ < 0) return false;
  int one = 1;
  setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(everywhere ? INADDR_ANY : INADDR_LOOPBACK);
  if (bind(listenFd_, (sockaddr *)&a, sizeof a) < 0 || ::listen(listenFd_, 8) < 0) {
    ::close(listenFd_);
    listenFd_ = -1;
    return false;
  }
  fcntl(listenFd_, F_SETFL, O_NONBLOCK);
  return true;
}

void Server::addPollFds(std::vector<pollfd> &fds) {
  if (listenFd_ >= 0) fds.push_back({listenFd_, POLLIN, 0});
  for (auto &kv : conns_) fds.push_back({kv.first, (short)(POLLIN | (kv.second.out.empty() ? 0 : POLLOUT)), 0});
}

void Server::handle(const std::vector<pollfd> &fds) {
  for (const pollfd &p : fds) {
    if (!p.revents) continue;
    if (p.fd == listenFd_) {
      accept();
      continue;
    }
    auto it = conns_.find(p.fd);
    if (it == conns_.end()) continue;
    if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      close(p.fd);
      continue;
    }
    if (p.revents & POLLOUT) flush(it->second);
    if (conns_.count(p.fd) && (p.revents & POLLIN)) readFrom(conns_[p.fd]);
  }
}

void Server::accept() {
  int fd;
  while ((fd = ::accept(listenFd_, nullptr, nullptr)) >= 0) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    Conn c;
    c.fd = fd;
    c.id = nextId_++;
    conns_[fd] = c;
  }
}

void Server::readFrom(Conn &c) {
  char buf[8192];
  ssize_t n = ::read(c.fd, buf, sizeof buf);
  if (n <= 0) {
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
    close(c.fd);
    return;
  }
  c.in.append(buf, n);
  int fd = c.fd;
  if (c.ws)
    wsFrames(c);
  else
    httpRequest(c);
  if (conns_.count(fd) && conns_[fd].in.size() > 2 * kMaxFrame) close(fd);
}

void Server::flush(Conn &c) {
  while (!c.out.empty()) {
    ssize_t n = ::write(c.fd, c.out.data(), c.out.size());
    if (n <= 0) {
      if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
      close(c.fd);
      return;
    }
    c.out.erase(0, n);
  }
  if (c.closing) close(c.fd);
}

void Server::reply(Conn &c, int code, const char *status, const std::string &type, const std::string &body) {
  char head[256];
  snprintf(head, sizeof head,
           "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nCache-Control: no-store\r\n"
           "Connection: close\r\n\r\n",
           code, status, type.c_str(), body.size());
  c.out += head;
  c.out += body;
  c.closing = true;
  flush(c);
}

void Server::httpRequest(Conn &c) {
  size_t end = c.in.find("\r\n\r\n");
  if (end == std::string::npos) return;
  std::string req = c.in.substr(0, end + 4);
  c.in.erase(0, end + 4);
  std::istringstream first(req);
  std::string method, path;
  first >> method >> path;
  size_t q = path.find('?');
  if (q != std::string::npos) path.resize(q);

  if (path == "/ws") {
    std::string key = header(req, "Sec-WebSocket-Key");
    if (key.empty() || method != "GET") return reply(c, 400, "Bad Request", "text/plain", "WebSocket only\n");
    if (wsCount() >= kMaxWebSockets) return reply(c, 503, "Busy", "text/plain", "Too many clients\n");
    std::string accept = base64(sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
    c.out += "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
             "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    c.ws = true;
    int fd = c.fd, id = c.id;
    flush(c);
    if (!conns_.count(fd)) return;
    if (onOpen) onOpen(id);
    if (conns_.count(fd) && !conns_[fd].in.empty()) wsFrames(conns_[fd]);
    return;
  }
  if (method != "GET") return reply(c, 405, "Method Not Allowed", "text/plain", "GET only\n");
  if (path == "/favicon.ico") return reply(c, 204, "No Content", "text/plain", "");
  if (path.find("..") != std::string::npos) return reply(c, 400, "Bad Request", "text/plain", "Bad path\n");
  std::string file;
  if (path == "/sim" || path == "/sim.html") {
    file = webDir_ + "/sim.html";
  } else {
    file = appDir_ + (path == "/" ? "/index.html" : path);
  }
  std::ifstream f(file, std::ios::binary);
  if (!f && path == "/")
    return reply(c, 404, "Not Found", "text/html; charset=utf-8",
                 "<p>The phone app is not built: run <code>npm run build</code> in <code>app/</code>. "
                 "The simulator page is at <a href=\"/sim\">/sim</a>.</p>\n");
  if (!f) return reply(c, 404, "Not Found", "text/plain", "Not found: " + path + "\n");
  std::ostringstream body;
  body << f.rdbuf();
  reply(c, 200, "OK", contentType(file), body.str());
}

void Server::wsFrames(Conn &c) {
  for (;;) {
    std::string &in = c.in;
    if (in.size() < 2) return;
    uint8_t b0 = in[0], b1 = in[1];
    int opcode = b0 & 0x0F;
    bool masked = b1 & 0x80;
    uint64_t len = b1 & 0x7F;
    size_t pos = 2;
    if (len == 126) {
      if (in.size() < 4) return;
      len = (uint8_t)in[2] << 8 | (uint8_t)in[3];
      pos = 4;
    } else if (len == 127) {
      if (in.size() < 10) return;
      len = 0;
      for (int i = 0; i < 8; i++) len = len << 8 | (uint8_t)in[2 + i];
      pos = 10;
    }
    if (len > kMaxFrame || !(b0 & 0x80) || !masked) {  // too big, fragmented or unmasked
      close(c.fd);
      return;
    }
    if (in.size() < pos + 4 + len) return;
    const char *mask = &in[pos];
    std::string payload = in.substr(pos + 4, len);
    for (size_t i = 0; i < payload.size(); i++) payload[i] ^= mask[i % 4];
    in.erase(0, pos + 4 + len);

    if (opcode == 1) {
      int fd = c.fd;
      if (onMessage) onMessage(c.id, payload);
      if (!conns_.count(fd)) return;
    } else if (opcode == 8) {
      c.out += std::string("\x88\x00", 2);
      c.closing = true;
      flush(c);
      return;
    } else if (opcode == 9) {
      std::string pong;
      pong += (char)0x8A;
      pong += (char)payload.size();
      pong += payload.substr(0, 125);
      c.out += pong;
      flush(c);
    }
  }
}

void Server::send(int client, const std::string &text) {
  for (auto &kv : conns_) {
    Conn &c = kv.second;
    if (c.id != client || !c.ws || c.closing) continue;
    std::string frame;
    frame += (char)0x81;
    if (text.size() < 126) {
      frame += (char)text.size();
    } else if (text.size() < 65536) {
      frame += (char)126;
      frame += (char)(text.size() >> 8);
      frame += (char)(text.size() & 0xFF);
    } else {
      frame += (char)127;
      for (int i = 7; i >= 0; i--) frame += (char)((uint64_t)text.size() >> (i * 8));
    }
    c.out += frame + text;
    if (c.out.size() > (8u << 20)) {  // a client that stopped reading
      close(c.fd);
      return;
    }
    flush(c);
    return;
  }
}

void Server::close(int fd) {
  auto it = conns_.find(fd);
  if (it == conns_.end()) return;
  bool ws = it->second.ws;
  int id = it->second.id;
  ::close(fd);
  conns_.erase(it);
  if (ws && onClose) onClose(id);
}

int Server::wsCount() const {
  int n = 0;
  for (auto &kv : conns_) n += kv.second.ws && !kv.second.closing;
  return n;
}

}  // namespace sim
