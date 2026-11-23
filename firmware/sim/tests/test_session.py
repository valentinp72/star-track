#!/usr/bin/env python3
"""A scripted night on the simulator, through the protocol on stdin/stdout.

Usage: test_session.py path/to/star-track-sim

The mount is set up with a tilted base, an unknown north and a tube 4° off level
at power-on. The script aligns
on three stars, goes to M42 and tracks it for 4 hours of simulated time, then
checks the E-stop, jogs, the hand unit, park, a position table, the GPS and the
error replies. The simulator's sim_truth gives where the tube really points.
"""

import json
import math
import subprocess
import sys
from datetime import datetime, timezone

T0 = int(datetime(2026, 11, 21, 21, 0, tzinfo=timezone.utc).timestamp() * 1000)
LAT, LON = 45.0, 5.0

CAPELLA = (5.2782, 45.998)
ALDEBARAN = (4.5987, 16.509)
DENEB = (20.6905, 45.280)
M42 = (5.5881, -5.391)


class Sim:
    def __init__(self, path):
        self.p = subprocess.Popen([path, "--paused", "--port", "0", "--utc", str(T0)], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        self.next_id = 1
        self.logs = []

    def send(self, msg):
        self.p.stdin.write(json.dumps(msg) + "\n")
        self.p.stdin.flush()

    def cmd(self, cmd_name, **args):
        i = self.next_id
        self.next_id += 1
        self.send({"cmd": cmd_name, "id": i, **args})
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError("simulator exited")
            m = json.loads(line)
            if m.get("ev") == "log":
                self.logs.append(m)
            elif m.get("re") == i or (m.get("re") is None and "ok" in m):
                return m

    def ok(self, cmd_name, **args):
        r = self.cmd(cmd_name, **args)
        if not r["ok"]:
            raise AssertionError(f"{cmd_name} failed: {r}")
        return r

    def run(self, seconds):
        self.ok("sim_run", seconds=seconds)

    def status(self):
        return self.ok("status")

    def wait_mode(self, modes, timeout, step=0.5):
        if isinstance(modes, str):
            modes = (modes,)
        t = 0.0
        while t <= timeout:
            s = self.status()
            if s["mode"] in modes:
                return t
            self.run(step)
            t += step
        raise AssertionError(f"mode {modes} not reached in {timeout} s (now {s['mode']})")

    def pointing_error(self, ra, dec):
        t = self.ok("sim_truth")
        return separation(t["ra"], t["dec"], ra, dec)

    def close(self):
        self.p.stdin.close()
        self.p.wait(timeout=5)


def separation(ra1, dec1, ra2, dec2):
    """Angle between two RA (hours) / Dec (degrees) positions, in degrees."""
    a1, d1, a2, d2 = (math.radians(ra1 * 15), math.radians(dec1), math.radians(ra2 * 15), math.radians(dec2))
    c = math.sin(d1) * math.sin(d2) + math.cos(d1) * math.cos(d2) * math.cos(a1 - a2)
    return math.degrees(math.acos(max(-1.0, min(1.0, c))))


checks = []


def check(name, cond, detail=""):
    checks.append((name, cond))
    print(f"{'ok  ' if cond else 'FAIL'} {name}{'  (' + detail + ')' if detail else ''}")


def arcmin(deg):
    return f"{deg * 60:.2f}'"


def align_and_goto(s):
    # The tube far off level at power-on (32° up): the first star shows it.
    s.ok("sim_mount", tilt=1.5, tilt_az=70, north=140, alt_zero=32)
    s.ok("set_time", utc=T0)
    s.ok("set_site", lat=LAT, lon=LON)
    s.ok("sim_point", ra=CAPELLA[0], dec=CAPELLA[1])
    r = s.cmd("align_add", ra=CAPELLA[0], dec=CAPELLA[1], name="Capella")
    check("a tube 32° off level at power-on is refused at the first star",
          r["err"] == "align_mismatch" and "set_alt_zero" in r["msg"], r.get("msg", ""))

    # From here, the tube was 4° above level at power-on: the alignment finds it.
    s.ok("sim_mount", alt_zero=4)

    # Before the alignment the tube's altitude is unknown: no limit on jogs.
    s.ok("sim_point", alt=1, az=200)
    s.ok("sim_hand", x=0, y=-1)
    s.run(3)
    s.ok("sim_hand", x=0, y=0)
    s.run(2)
    alt = s.ok("sim_truth")["alt"]
    check("before alignment, jogs are not stopped at alt.min", alt < -3, f"{alt:.2f}°")

    r = s.cmd("goto", ra=M42[0], dec=M42[1])
    check("goto before alignment is refused", r["err"] == "not_ready", r["msg"])

    # Star 1: the user centres Capella by hand.
    s.ok("sim_point", ra=CAPELLA[0], dec=CAPELLA[1])
    r = s.ok("align_add", ra=CAPELLA[0], dec=CAPELLA[1], name="Capella")
    check("first star", r["n"] == 1)
    check("first star: the altitude index is found (level base assumed)", abs(r["alt_index"] + 4) < 1.5,
          f"{r['alt_index']:.2f}°")

    # Star 2: goto with the one-star model (azimuth offset only), then centre.
    s.run(120)
    s.ok("goto", ra=ALDEBARAN[0], dec=ALDEBARAN[1], track=False)
    s.wait_mode("idle", 120)
    e1 = s.pointing_error(*ALDEBARAN)
    check("goto Aldebaran with one star lands within 5°", e1 < 5, arcmin(e1))
    s.ok("sim_point", ra=ALDEBARAN[0], dec=ALDEBARAN[1])
    s.ok("align_add", ra=ALDEBARAN[0], dec=ALDEBARAN[1], name="Aldebaran")

    # A star that is not centred (the tube is still on Aldebaran) is refused.
    r = s.cmd("align_add", ra=DENEB[0], dec=DENEB[1], name="Deneb")
    check("an alignment star that is not centred is refused", r["err"] == "align_mismatch", r["msg"])

    # Star 3: with two stars the tilt and north are known.
    s.run(120)
    s.ok("goto", ra=DENEB[0], dec=DENEB[1], track=False)
    s.wait_mode("idle", 180)
    e2 = s.pointing_error(*DENEB)
    check("goto Deneb with two stars lands within 2'", e2 < 2 / 60, arcmin(e2))
    s.ok("sim_point", ra=DENEB[0], dec=DENEB[1])
    r = s.ok("align_add", ra=DENEB[0], dec=DENEB[1], name="Deneb")
    check("three stars, residual under 1'", r["n"] == 3 and r["rms"] < 1 / 60, arcmin(r["rms"]))
    check("three stars: altitude index -4°", abs(r["alt_index"] + 4) < 0.02, f"{r['alt_index']:.3f}°")
    stars = s.ok("align_list")["stars"]
    check("align_list names the stars", [x["name"] for x in stars] == ["Capella", "Aldebaran", "Deneb"])


def track_m42(s):
    r = s.ok("goto", ra=M42[0], dec=M42[1])
    t = s.wait_mode("tracking", 180)
    check("goto M42 arrives and tracks", True, f"{t:.1f} s, eta said {r['eta']:.1f} s")
    worst = 0.0
    for _ in range(4 * 60):  # 4 hours, checked every minute
        s.run(60)
        worst = max(worst, s.pointing_error(*M42))
    st = s.status()
    check("still tracking after 4 h", st["mode"] == "tracking")
    check("tracking error over 4 h under 1'", worst < 1 / 60, "worst " + arcmin(worst))
    check("status pos agrees with the target", separation(st["pos"]["ra"], st["pos"]["dec"], *M42) < 1 / 60)


def jog_and_hand(s):
    s.ok("stop")
    check("stop ends tracking", s.status()["mode"] == "idle")

    # The jog's altitude limit is on the sky once aligned: the tube was 4° up at
    # power-on, so the axis reads about -4° at the horizon. Full speed down from
    # 6°: it brakes in time.
    s.ok("sim_point", alt=6, az=200)
    s.ok("sim_hand", x=0, y=-1)
    s.run(8)
    s.ok("sim_hand", x=0, y=0)
    s.run(2)
    alt = s.ok("sim_truth")["alt"]
    check("a jog down stops at alt.min on the sky, not on the axis", -0.3 < alt < 0.5, f"{alt:.2f}°")
    s.send({"cmd": "jog", "x": 1, "y": 0})  # no id: no reply
    s.run(0.1)
    check("jog starts manual mode", s.status()["mode"] == "manual")
    s.run(1)
    check("jog expires after 200 ms (dead-man)", s.status()["mode"] == "idle")

    # A diagonal push moves each axis as a straight push of the same length.
    for x, want in ((0.7071, 3.0), (0.3536, 0.75)):
        for _ in range(30):
            s.send({"cmd": "jog", "x": x, "y": x})
            s.run(0.1)
        ax = s.status()["axes"]
        s.ok("stop")
        s.run(3)
        check(f"a 45° jog of length {x * 2 ** 0.5:.1f} runs both axes at {want}°/s",
              abs(abs(ax["az"]["speed"]) - want) < 0.05 and abs(abs(ax["alt"]["speed"]) - want) < 0.05,
              f"az {ax['az']['speed']:.2f}, alt {ax['alt']['speed']:.2f}°/s")

    s.ok("sim_hand", x=0.8, y=0)
    s.run(0.2)
    st = s.status()
    check("hand joystick jogs", st["mode"] == "manual" and st["hand"])
    s.ok("sim_hand", x=0, y=0)
    s.run(0.2)
    check("hand joystick released", s.status()["mode"] == "idle")

    # Turned off: the joystick is ignored, a goto runs on.
    s.ok("set_config", config={"hand.enabled": False})
    s.ok("goto", ra=M42[0], dec=M42[1])
    s.run(0.5)
    s.ok("sim_hand", x=0.8, y=0)
    s.run(0.5)
    st = s.status()
    check("hand unit off: its joystick is ignored", st["mode"] == "goto" and not st["hand"], st["mode"])
    s.ok("sim_hand", x=0, y=0)
    s.ok("stop")
    s.wait_mode("idle", 30)
    s.ok("set_config", config={"hand.enabled": True})

    # Jog while tracking: the mount moves off the target and keeps tracking.
    s.ok("goto", ra=M42[0], dec=M42[1])
    s.wait_mode("tracking", 120)
    s.ok("sim_hand", x=0.5, y=0)
    s.run(1)
    check("jog while tracking shows manual", s.status()["mode"] == "manual")
    s.ok("sim_hand", x=0, y=0)
    s.run(0.5)
    check("tracking resumes after the jog", s.status()["mode"] == "tracking")
    s.run(30)
    off = s.pointing_error(*M42)
    s.run(300)
    drift = abs(s.pointing_error(*M42) - off)
    check("the jog offset is kept while tracking", off > 0.05 and drift < 1 / 60,
          f"offset {arcmin(off)}, drift {arcmin(drift)}")

    s.ok("sim_hand", buttons=[True, False, False])
    s.run(0.2)
    s.ok("sim_hand", buttons=[False, False, False])
    check("hand button 1 stops", s.status()["mode"] == "idle")


def estop_and_park(s):
    s.ok("goto", ra=M42[0], dec=M42[1])
    s.wait_mode("tracking", 120)
    s.ok("sim_estop", pressed=True)
    s.run(0.2)
    st = s.status()
    check("E-stop: mode estop, position uncertain", st["mode"] == "estop" and not st["pos_ok"] and st["estop"])
    check("E-stop: drivers off", not s.ok("sim_truth")["drivers"])
    r = s.cmd("goto", ra=M42[0], dec=M42[1])
    check("goto refused during E-stop", r["err"] == "estop")
    r = s.cmd("arm")
    check("arm refused while pressed", r["err"] == "estop")
    s.ok("sim_estop", pressed=False)
    s.ok("arm")
    st = s.status()
    check("arm after release: idle, still uncertain", st["mode"] == "idle" and not st["pos_ok"])
    r = s.cmd("park")
    check("park needs a known position", r["err"] == "not_ready")

    s.ok("sim_point", ra=M42[0], dec=M42[1])
    s.ok("sync", ra=M42[0], dec=M42[1])
    check("sync restores pos_ok", s.status()["pos_ok"])
    s.ok("park")
    s.wait_mode("parked", 240)
    st = s.status()
    az = st["axes"]["az"]["steps"] % 160000
    check("parked at the park position", abs(st["axes"]["alt"]["steps"]) < 20 and min(az, 160000 - az) < 20,
          f"alt {st['axes']['alt']['steps']}, az {st['axes']['az']['steps']} steps")
    r = s.cmd("goto", ra=M42[0], dec=M42[1])
    check("goto refused while parked", r["err"] == "parked")
    s.ok("unpark")
    check("unpark", s.status()["mode"] == "idle")


def table(s):
    now = s.ok("sim_truth")["utc"]

    def pos(t):  # a moving object: 0.03 h/h in RA with some curvature, 0.3°/h in Dec
        h = (t - now) / 3.6e6
        return 5.9 + 0.03 * h + 0.002 * h * h, 2.0 + 0.3 * h

    points = [[now + k * 600000 - 600000, *pos(now + k * 600000 - 600000)] for k in range(20)]
    r = s.ok("track_table", points=points)
    s.wait_mode("tracking", 120)
    worst = 0.0
    for _ in range(60):
        s.run(60)
        t = s.ok("sim_truth")
        worst = max(worst, separation(t["ra"], t["dec"], *pos(t["utc"])))
    check("track_table follows a moving object within 1'", worst < 1 / 60, "worst " + arcmin(worst))
    s.run(3 * 3600)
    check("tracking stops at the end of the table", s.status()["mode"] == "idle")
    check("a warning is logged", any("table" in m["msg"] for m in s.logs))


def gps(s):
    s.ok("sim_gps", on=True, fix=True, lat=LAT, lon=LON, sats=9)
    s.run(2.5)
    st = s.status()
    check("GPS gives time and site", st["time_src"] == "gps" and st["site_src"] == "gps" and st["gps"]["fix"],
          f"{st['time_src']}, {st['site_src']}, {st['gps']}")
    truth = s.ok("sim_truth")["utc"]
    check("GPS time matches true UTC within 1 s", abs(st["utc"] - truth) <= 1000, f"{st['utc'] - truth} ms")
    r = s.ok("set_time", utc=T0)
    check("set_time ignored with a GPS fix", r["source"] == "gps")
    s.ok("sim_gps", on=False)


def errors(s):
    s.p.stdin.write("{not json\n")
    s.p.stdin.flush()
    r = json.loads(s.p.stdout.readline())
    check("bad_json", r["err"] == "bad_json" and r["re"] is None)
    check("unknown_cmd", s.cmd("nope")["err"] == "unknown_cmd")
    check("unknown sim command", s.cmd("sim_nope")["err"] == "unknown_cmd")
    r = s.cmd("goto", ra=30, dec=0)
    check("bad_arg names the argument", r["err"] == "bad_arg" and "ra" in r["msg"], r["msg"])
    r = s.cmd("set_config", config={"alt.min": 5, "nope": 1})
    check("set_config refuses an unknown key", r["err"] == "bad_arg")
    s.ok("set_config", config={"alt.min": 5})
    check("set_config / get_config", s.ok("get_config", keys=["alt.min"])["config"] == {"alt.min": 5})
    r = s.cmd("goto_altaz", alt=2)
    check("below_limit", r["err"] == "below_limit")
    h = s.ok("hello", proto=1)
    check("hello", h["proto"] == 1 and h["sim"] and h["board"] == "sim")


def sep_altaz(alt1, az1, alt2, az2):
    """Angle between two alt/az directions, in degrees."""
    a1, z1, a2, z2 = map(math.radians, (alt1, az1, alt2, az2))
    c = math.sin(a1) * math.sin(a2) + math.cos(a1) * math.cos(a2) * math.cos(z1 - z2)
    return math.degrees(math.acos(max(-1.0, min(1.0, c))))


def sun_safety(s):
    day = T0 + 14 * 3600 * 1000  # the next day, 11:00 UTC
    s.ok("set_config", config={"alt.min": 0})
    s.run(5)  # the GPS (turned off before) no longer counts as a fix
    s.ok("sim_clock", utc=day)
    r = s.ok("set_time", utc=day)
    check("phone time accepted once the GPS is off", r["source"] == "phone")
    sun = s.status()["sun"]
    check("the Sun is up at 11:00 UTC", sun["alt"] > 10, f"alt {sun['alt']:.1f}°")

    r = s.cmd("goto_altaz", alt=sun["alt"] + 5, az=sun["az"])
    check("goto 5° from the Sun is refused", r["err"] == "sun", r["msg"])
    s.ok("sim_point", alt=sun["alt"], az=sun["az"] - 40)
    r = s.cmd("goto_altaz", alt=sun["alt"], az=sun["az"] + 40)
    check("a slew across the Sun is refused", r["err"] == "sun" and "slew" in r["msg"], r["msg"])

    s.ok("sim_hand", x=1, y=0)  # jog towards the Sun (azimuth increasing)
    s.run(30)
    st = s.status()
    s.ok("sim_hand", x=0, y=0)
    s.run(1)
    check("a jog towards the Sun stops at the edge of the zone", 15 <= st["sun"]["sep"] < 17,
          f"{st['sun']['sep']:.2f}° from the Sun")
    check("the jog stop is logged", any("jog stopped" in m["msg"] for m in s.logs))
    s.ok("sim_hand", x=-1, y=0)  # away from the Sun: allowed
    s.run(2)
    s.ok("sim_hand", x=0, y=0)
    s.run(1)
    check("jogging away from the Sun works", s.status()["sun"]["sep"] > st["sun"]["sep"] + 3)

    r = s.cmd("goto", sun=True)
    check("goto the Sun needs solar mode", r["err"] == "sun")
    r = s.cmd("solar", on=True)
    check("solar mode needs the filter confirmation", r["err"] == "bad_arg" and "filter" in r["msg"])
    s.ok("solar", on=True, filter=True)
    check("status shows solar mode", s.status()["solar"])
    s.ok("goto", sun=True)
    s.wait_mode("tracking", 120)
    s.run(600)
    st, t = s.status(), s.ok("sim_truth")
    e = sep_altaz(t["alt"], t["az"], st["sun"]["alt"], st["sun"]["az"])
    check("solar mode: tracks the Sun", st["mode"] == "tracking" and e < 1 / 60, arcmin(e))
    s.ok("solar", on=False)
    s.run(0.5)
    check("solar mode off stops following the Sun", s.status()["mode"] == "idle")
    sun = s.status()["sun"]
    r = s.cmd("goto_altaz", alt=sun["alt"] + 3, az=sun["az"])
    check("from the Sun, a slew deeper into the zone is refused", r["err"] == "sun")
    r = s.cmd("goto_altaz", alt=sun["alt"] + 25, az=sun["az"])
    check("from the Sun, a slew straight out of the zone is allowed", r["ok"], r.get("msg", ""))
    s.wait_mode("idle", 60)
    s.ok("solar", on=True, filter=True)
    s.ok("park")
    s.wait_mode("parked", 240)
    check("park turns solar mode off", not s.status()["solar"])
    s.ok("unpark")

    # Not aligned with the Sun up: one warning, not one per control step.
    s.ok("align_clear")
    s.logs.clear()
    s.ok("sim_hand", x=0.5, y=0)
    s.run(3)
    s.ok("sim_hand", x=0, y=0)
    s.run(1)
    s.ok("sim_hand", x=-0.5, y=0)
    s.run(3)
    s.ok("sim_hand", x=0, y=0)
    s.run(1)
    n = sum("not aligned" in m["msg"] for m in s.logs)
    check("the unaligned-Sun warning is logged once, not at every step", n == 1, f"{n} times")


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    s = Sim(sys.argv[1])
    try:
        align_and_goto(s)
        track_m42(s)
        jog_and_hand(s)
        estop_and_park(s)
        table(s)
        gps(s)
        errors(s)
        sun_safety(s)
    finally:
        s.close()
    failed = [n for n, ok in checks if not ok]
    print(f"test_session: {len(checks)} checks, {len(failed)} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
