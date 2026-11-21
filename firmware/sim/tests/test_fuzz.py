#!/usr/bin/env python3
"""Random sequences of user actions on the simulator, with invariants checked
after every step.

Usage: test_fuzz.py path/to/star-track-sim [seeds] [actions]

Each seed sets up a base with a random tilt and north, and a tube up to 5° off
level at power-on, aligns on three stars,
then runs random actions: gotos, gotos to alt/az, hand-unit and app jogs, time
jumps, tracking on and off, stop, park, position tables, centre-and-sync,
centre-and-align, E-stops with their recovery, setting changes. After every
action, sim_state must show:

- the firmware's belief within 1' of where the tube really points (the
  simulator only loses steps when told to, so any gap is a bug);
- a known mode, a target in goto and tracking, axis speeds within the limits;
- a reply to every command with an id, with "ok" and, when refused, an error
  code from docs/protocol.md.
"""

import json
import os
import random
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_session import T0  # noqa: E402

STARS = [
    ("Sirius", 6.7525, -16.716), ("Arcturus", 14.261, 19.182), ("Vega", 18.6156, 38.784),
    ("Capella", 5.2782, 45.998), ("Rigel", 5.2423, -8.202), ("Procyon", 7.655, 5.225),
    ("Betelgeuse", 5.9195, 7.407), ("Altair", 19.8464, 8.868), ("Aldebaran", 4.5987, 16.509),
    ("Pollux", 7.7553, 28.026), ("Deneb", 20.6905, 45.28), ("Regulus", 10.1395, 11.967),
    ("Castor", 7.5767, 31.888), ("Mirfak", 3.4054, 49.861), ("Dubhe", 11.0621, 61.751),
    ("Alpheratz", 0.1398, 29.091), ("Hamal", 2.1196, 23.463), ("Schedar", 0.6751, 56.537),
    ("M31", 0.7123, 41.269), ("M42", 5.5881, -5.391), ("M45", 3.7914, 24.105), ("M13", 16.6948, 36.461),
    ("M81", 9.9258, 69.065), ("M44", 8.67, 19.667), ("M57", 18.8932, 33.029), ("Polaris", 2.5303, 89.264),
]
ERRORS = {"bad_json", "too_long", "unknown_cmd", "bad_arg", "estop", "parked", "not_ready", "below_limit", "sun",
          "align_mismatch"}
MODES = {"estop", "idle", "manual", "goto", "tracking", "parked"}
LIMIT = 1 / 60  # degrees


class Fail(Exception):
    pass


class Sim:
    def __init__(self, path, utc):
        self.p = subprocess.Popen([path, "--paused", "--port", "0", "--utc", str(utc)], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        self.n = 0
        self.history = []

    def cmd(self, cmd_name, **args):
        self.n += 1
        msg = {"cmd": cmd_name, "id": self.n, **args}
        self.history.append(msg)
        self.p.stdin.write(json.dumps(msg) + "\n")
        self.p.stdin.flush()
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise Fail(f"the simulator exited after {msg}")
            m = json.loads(line)
            if m.get("re") == self.n:
                if "ok" not in m or (not m["ok"] and m.get("err") not in ERRORS):
                    raise Fail(f"bad reply to {msg}: {m}")
                return m

    def ok(self, cmd_name, **args):
        r = self.cmd(cmd_name, **args)
        if not r["ok"]:
            raise Fail(f"{cmd_name} {args} refused: {r}")
        return r

    def send(self, cmd_name, **args):  # no id, no reply (jog)
        msg = {"cmd": cmd_name, **args}
        self.history.append(msg)
        self.p.stdin.write(json.dumps(msg) + "\n")
        self.p.stdin.flush()

    def close(self):
        self.p.stdin.close()
        self.p.wait(timeout=10)


def check_state(s, where, cfg):
    st = s.ok("sim_state")
    status, truth = st["status"], st["truth"]
    if status["mode"] not in MODES:
        raise Fail(f"{where}: unknown mode {status['mode']}")
    if status["mode"] in ("goto", "tracking") and status["target"] is None:
        raise Fail(f"{where}: {status['mode']} without a target")
    limit = max(cfg["slew.speed"], cfg["jog.max_speed"])
    for axis in ("az", "alt"):
        if abs(status["axes"][axis]["speed"]) > limit * 1.01 + 1e-6:
            raise Fail(f"{where}: {axis} speed {status['axes'][axis]['speed']} over the limit {limit}")
    if os.environ.get("FUZZ_TRACE"):
        me = st["model_error"]
        print(f"  {where}: mode {status['mode']}, model error {'-' if me is None else f'{me * 3600:.1f}″'}, "
              f"pos_ok {status['pos_ok']}")
    if status["mode"] == "estop" or not status["pos_ok"]:
        return st
    if st["model_error"] is not None and st["model_error"] > LIMIT:
        raise Fail(f"{where}: the firmware is {st['model_error'] * 60:.2f}' from where the tube points "
                   f"(firmware {status['pos']}, truth alt {truth['alt']:.4f} az {truth['az']:.4f})")
    return st


def wait_settled(s, timeout):
    """Runs until the mount is no longer slewing (or the timeout)."""
    t = 0
    while t < timeout:
        mode = s.ok("status")["mode"]
        if mode not in ("goto", "manual"):
            return mode
        s.ok("sim_run", seconds=2)
        t += 2
    return s.ok("status")["mode"]


def run_seed(path, seed, actions):
    rng = random.Random(seed)
    utc = T0 + rng.randrange(0, 24) * 3600 * 1000
    s = Sim(path, utc)
    cfg = {"slew.speed": 3.0, "jog.max_speed": 3.0}
    log = []
    try:
        # A tilted base, north anywhere, and the tube up to 5° off level at power-on.
        s.ok("sim_mount", tilt=rng.uniform(0, 3), tilt_az=rng.uniform(0, 360), north=rng.uniform(0, 360),
             alt_zero=rng.uniform(-5, 5))
        s.ok("set_time", utc=utc)
        s.ok("set_site", lat=45, lon=5)
        s.ok("set_config", config={"sun.min_sep": 5})  # leave room to move by day

        def visible(name, ra, dec, low=12):
            s.ok("sim_point", ra=ra, dec=dec)
            return s.ok("sim_truth")["alt"] > low

        def centre_and(kind, star):
            name, ra, dec = star
            if not visible(name, ra, dec, 5):
                return "below"
            r = s.cmd(kind, ra=ra, dec=dec, name=name) if kind == "align_add" else s.cmd(kind, ra=ra, dec=dec)
            if not r["ok"] and r["err"] == "bad_arg" and "zenith" in r["msg"]:
                return "zenith"
            if not r["ok"]:
                raise Fail(f"{kind} on a centred {name} refused: {r}")
            return "ok"

        # Three alignment stars spread over the sky.
        stars = [x for x in STARS if visible(*x, low=15)]
        rng.shuffle(stars)
        chosen = []
        for x in stars:
            s.ok("sim_point", ra=x[1], dec=x[2])
            az = s.ok("sim_truth")["az"]
            if all(abs((az - a + 180) % 360 - 180) > 40 for a in chosen_az(chosen)):
                chosen.append((x, az))
            if len(chosen) == 3:
                break
        for (x, _) in chosen:
            centre_and("align_add", x)
        check_state(s, "after the alignment", cfg)

        estopped = False
        for i in range(actions):
            act = rng.choice(["goto", "goto", "altaz", "hand", "appjog", "run", "track", "stop", "park", "table",
                              "sync", "align", "estop", "config", "rate"])
            where = f"seed {seed}, action {i} ({act})"
            log.append(act)
            if act == "goto":
                name, ra, dec = rng.choice(STARS)
                s.cmd("goto", ra=ra, dec=dec, track=rng.random() < 0.7)
                s.ok("sim_run", seconds=rng.choice([0.3, 2, 10, 60]))
            elif act == "altaz":
                s.cmd("goto_altaz", alt=rng.uniform(5, 85), az=rng.uniform(0, 360))
                s.ok("sim_run", seconds=rng.choice([0.5, 5, 60]))
            elif act == "hand":
                s.ok("sim_hand", x=rng.uniform(-1, 1), y=rng.uniform(-1, 1))
                s.ok("sim_run", seconds=rng.uniform(0.1, 4))
                check_state(s, where + " during the jog", cfg)
                s.ok("sim_hand", x=0, y=0)
                s.ok("sim_run", seconds=1)
            elif act == "appjog":
                for _ in range(rng.randrange(1, 8)):
                    s.send("jog", x=round(rng.uniform(-1, 1), 2), y=round(rng.uniform(-1, 1), 2))
                    s.ok("sim_run", seconds=0.1)
                s.ok("sim_run", seconds=1)
            elif act == "run":
                s.ok("sim_run", seconds=rng.choice([1, 30, 300, 1800]))
            elif act == "track":
                s.cmd("track", on=rng.random() < 0.6)
                s.ok("sim_run", seconds=5)
            elif act == "stop":
                s.ok("stop")
                s.ok("sim_run", seconds=3)
            elif act == "park":
                if s.cmd("park")["ok"]:
                    wait_settled(s, 300)
                    check_state(s, where + " parked", cfg)
                    s.ok("unpark")
            elif act == "table":
                now = s.ok("sim_truth")["utc"]
                ra0, dec0 = rng.uniform(0, 24), rng.uniform(-10, 70)
                pts = [[now - 600000 + k * 600000, (ra0 + 0.02 * k) % 24, dec0 + 0.05 * k] for k in range(20)]
                s.cmd("track_table", points=pts)
                s.ok("sim_run", seconds=rng.choice([5, 60, 600]))
            elif act == "sync":
                s.ok("stop")
                centre_and("sync", rng.choice(STARS))
            elif act == "align":
                s.ok("stop")
                centre_and("align_add", rng.choice(STARS))
            elif act == "estop":
                s.ok("sim_estop", pressed=True)
                s.ok("sim_run", seconds=rng.uniform(0.01, 3))
                check_state(s, where + " pressed", cfg)
                s.ok("sim_estop", pressed=False)
                s.ok("arm")
                # Recovery: centre a star and sync on it, as the page's panel does.
                for x in rng.sample(STARS, len(STARS)):
                    if centre_and("sync", x) == "ok":
                        break
                estopped = True
            elif act == "config":
                old = max(cfg["slew.speed"], cfg["jog.max_speed"])
                cfg["slew.speed"] = round(rng.uniform(0.5, 6), 2)
                cfg["jog.max_speed"] = round(rng.uniform(0.5, 5), 2)
                accel = round(rng.uniform(0.5, 5), 2)
                s.ok("set_config", config={"slew.speed": cfg["slew.speed"], "slew.accel": accel,
                                           "jog.max_speed": cfg["jog.max_speed"]})
                s.ok("sim_run", seconds=old / accel + 1)  # time to slow down to the new limit
            elif act == "rate":
                s.ok("sim_clock", rate=rng.choice([1, 10, 1000]))  # no effect while paused, must not break
                s.ok("sim_clock", rate=0)
            check_state(s, where, cfg)
        return None
    except Fail as e:
        return f"{e}\n  last actions: {log[-8:]}\n  last messages: {s.history[-6:]}"
    finally:
        s.close()


def chosen_az(chosen):
    return [az for (_, az) in chosen]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    seeds = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    actions = int(sys.argv[3]) if len(sys.argv) > 3 else 150
    failed = 0
    for seed in range(seeds):
        err = run_seed(sys.argv[1], seed, actions)
        if err:
            failed += 1
            print(f"FAIL seed {seed}: {err}")
        else:
            print(f"ok   seed {seed}: {actions} actions")
    print(f"test_fuzz: {seeds} seeds, {failed} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
