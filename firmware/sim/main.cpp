// Simulator entry point: runs the mount firmware's core on the PC, with the
// protocol on stdin/stdout (one JSON object per line).
//
//   star-track-sim [--paused] [--rate R] [--utc MS] [--state FILE]
//
// --paused   the clock only moves with sim_run (for scripted tests)
// --rate R   clock speed (default 1)
// --utc MS   true UTC at start, ms since 1970 (default: the PC's clock)
// --state F  keep the flash records in file F between runs

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <string>

#include "sim.h"

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
  fprintf(stderr, "usage: star-track-sim [--paused] [--rate R] [--utc MS] [--state FILE]\n");
  exit(2);
}

}  // namespace

int main(int argc, char **argv) {
  double rate = 1;
  int64_t utc = wallMs();
  std::string state;
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
    } else {
      usage();
    }
  }

  sim::Sim s(state);
  s.rate = rate;
  s.begin(utc);

  std::string buf;
  int64_t last = steadyMs();
  double pending = 0;
  for (;;) {
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    int r = poll(&pfd, 1, s.rate > 0 ? sim::Sim::kStepMs : 100);
    if (r > 0) {
      char chunk[4096];
      ssize_t n = read(STDIN_FILENO, chunk, sizeof chunk);
      if (n <= 0) break;  // end of input
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
    int64_t t = steadyMs();
    pending += (t - last) * s.rate;
    last = t;
    if (pending >= 1) {
      int64_t ms = (int64_t)pending;
      pending -= ms;
      s.advance(ms);
    }
  }
  return 0;
}
