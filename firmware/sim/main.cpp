// Simulator entry point: runs the mount firmware's core on the PC, with the
// protocol on stdin/stdout (one JSON object per line) and on a WebSocket at
// ws://localhost:8080/ws. The phone app is at http://localhost:8080/ and the
// simulator page at http://localhost:8080/sim.
//
//   star-track-sim [--paused] [--rate R] [--utc MS] [--state FILE]
//                  [--port N] [--any] [--web DIR] [--app DIR]
//
// --paused   the clock only moves with sim_run (for scripted tests)
// --rate R   clock speed (default 1)
// --utc MS   true UTC at start, ms since 1970 (default: the PC's clock)
// --state F  keep the flash records in file F between runs
// --port N   HTTP and WebSocket port (default 8080; 0 = no server)
// --any      listen on every network interface, not only this PC
// --web DIR  directory of the simulator page (default: firmware/sim/web)
// --app DIR  the phone app's build (default: app/dist)

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <vector>

#include "server.h"
#include "sim.h"

#ifndef SIM_WEB_DIR
#define SIM_WEB_DIR "web"
#endif
#ifndef SIM_APP_DIR
#define SIM_APP_DIR "../../app/dist"
#endif

namespace {

int64_t wallMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int64_t steadyMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void usage() {
  fprintf(stderr,
          "usage: star-track-sim [--paused] [--rate R] [--utc MS] [--state FILE]\n"
          "                      [--port N] [--any] [--web DIR] [--app DIR]\n");
  exit(2);
}

}  // namespace

int main(int argc, char **argv) {
  double rate = 1;
  int64_t utc = wallMs();
  std::string state, web = SIM_WEB_DIR, app = SIM_APP_DIR;
  int port = 8080;
  bool any = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--paused") {
      rate = 0;
    } else if (a == "--rate" && i + 1 < argc) {
      rate = atof(argv[++i]);
    } else if (a == "--utc" && i + 1 < argc) {
      utc = atoll(argv[++i]);
    } else if (a == "--state" && i + 1 < argc) {
      state = argv[++i];
    } else if (a == "--port" && i + 1 < argc) {
      port = atoi(argv[++i]);
    } else if (a == "--any") {
      any = true;
    } else if (a == "--web" && i + 1 < argc) {
      web = argv[++i];
    } else if (a == "--app" && i + 1 < argc) {
      app = argv[++i];
    } else {
      usage();
    }
  }

  sim::Sim s(state);
  s.rate = rate;
  s.begin(utc);

  sim::Server server;
  if (port > 0) {
    if (!server.listen(port, any, web, app)) {
      fprintf(stderr, "star-track-sim: cannot listen on port %d (in use?)\n", port);
      return 1;
    }
    server.onOpen = [&](int c) { s.connect(c); };
    server.onMessage = [&](int c, const std::string &text) { s.handleLine(c, text.c_str()); };
    server.onClose = [&](int c) { s.disconnect(c); };
    s.setServer(&server);
    const char *host = any ? "<this PC>" : "localhost";
    fprintf(stderr, "star-track-sim: phone app at http://%s:%d/, simulator page at http://%s:%d/sim\n", host, port,
            host, port);
  }

  bool stdinOpen = true;
  std::string buf;
  int64_t last = steadyMs();
  double pending = 0;
  for (;;) {
    std::vector<pollfd> fds;
    if (stdinOpen) fds.push_back({STDIN_FILENO, POLLIN, 0});
    server.addPollFds(fds);
    poll(fds.data(), fds.size(), s.effectiveRate() > 0 ? sim::Sim::kStepMs : 100);

    if (stdinOpen && fds[0].revents) {
      char chunk[4096];
      ssize_t n = read(STDIN_FILENO, chunk, sizeof chunk);
      if (n <= 0) {
        if (port <= 0) break;  // end of input, nothing else to serve
        stdinOpen = false;
      } else {
        buf.append(chunk, n);
        size_t nl;
        while ((nl = buf.find('\n')) != std::string::npos) {
          std::string line = buf.substr(0, nl);
          buf.erase(0, nl + 1);
          if (!line.empty() && line.back() == '\r') line.pop_back();
          s.handleLine(sim::Sim::kConsole, line.c_str());
        }
        if (buf.size() > 65536) buf.clear();  // runaway line without a newline
      }
    }
    server.handle(fds);

    int64_t t = steadyMs();
    pending += (t - last) * s.effectiveRate();
    last = t;
    if (pending >= 1) {
      // At most one real second of work per pass, so the page stays responsive.
      int64_t ms = (int64_t)pending;
      if (ms > 100000) ms = 100000;
      pending -= ms;
      if (pending > 100000) pending = 0;
      s.advance(ms);
    }
  }
  return 0;
}
