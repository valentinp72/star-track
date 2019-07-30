
import time
import datetime
import threading
import numpy as np
import wiringpi as wp

from queue import Queue
from logic.mapping import Mapping

class Axis:

    STEPS_PER_ROTATION = 200
    MICROSTEPPING      = 1/16
    GEAR_RATIO         = 20/250

    SECOND_ARC_PER_STEP = ((360 * 60 * 60) / (STEPS_PER_ROTATION / MICROSTEPPING)) * GEAR_RATIO

    def alitude():
        return Axis(Mapping.ALTITUDE_DIR, Mapping.ALTITUDE_STEP)

    def azimuth():
        return Axis(Mapping.AZIMUTH_DIR, Mapping.AZIMUTH_STEP)

    #########

    def __init__(self, dir_pin, step_pin):
        self.motor = Motor(dir_pin, step_pin)
        self.motor.start()

        self.current = 0

    def move_angle(self, degrees=0, minutes=0, seconds=0):
        total = seconds + 60 * (minutes + 60 * degrees)
        steps = int(1 / (Axis.SECOND_ARC_PER_STEP) * total)
        print(total, steps)
        self.motor.set_dist(steps)

    def set_position(self, steps):
        pass

class Motor(threading.Thread):

    DIR_FWD = 1
    DIR_BCK = -1

    ACCELERATION = 500  # 0.01
    MIN_SPEED    = 10  # 0.01
    MAX_SPEED    = 2000  # 0.0005

    def __init__(self, dir_pin, step_pin):
        super(Motor, self).__init__()
        self.dir_pin  = dir_pin
        self.step_pin = step_pin
        self.queue = Queue(2)

        # current direction
        self.dir = None

        # current speed
        self.speed = Motor.MIN_SPEED
        self.set_dir(Motor.DIR_FWD)

        # current and destination positions
        self.curr = 0
        self.dest = 0

        wp.pinMode(self.dir_pin,  wp.OUTPUT)
        wp.pinMode(self.step_pin, wp.OUTPUT)

    def set_dist(self, dist):
        if not self.queue.empty():
            self.queue.get()
        self.queue.put(dist)

    def set_dir(self, dir):
        if dir != Motor.DIR_FWD and dir != Motor.DIR_BCK:
            raise ValueError("dir {} error".format(dir))
        wp.digitalWrite(self.dir_pin, 0 if dir == Motor.DIR_FWD else 1)

    def run(self):
        self.last = datetime.datetime.now()
        while True:
            self.last = datetime.datetime.now()
            if not self.queue.empty():
                self.dest = self.queue.get()

                delta = self.dest - self.curr

                if delta == 0:
                    continue
                elif delta > 0:
                    self.dir = Motor.DIR_FWD
                elif delta < 0:
                    self.dir = Motor.DIR_BCK

                ramp_up   = np.linspace(self.speed, Motor.MAX_SPEED, num=Motor.ACCELERATION, dtype=int)
                ramp_down = np.linspace(Motor.MAX_SPEED, Motor.MIN_SPEED, num=Motor.ACCELERATION, dtype=int)
                ramp_steps = len(ramp_up) + len(ramp_down)

                if ramp_steps <= abs(delta):
                    full = np.array([Motor.MAX_SPEED for _ in range(abs(delta) - ramp_steps)])
                    self.speeds = np.concatenate([ramp_up, full, ramp_down])
                else:
                    missing_steps = int(abs(delta) / 2)

                    ramp_up   = ramp_up[:missing_steps]
                    ramp_down = np.linspace(ramp_up[-1], Motor.MIN_SPEED, num=len(ramp_up))

                    full = [] if delta % 2 == 0 else [ramp_up[-1]]

                    self.speeds = np.concatenate([ramp_up, full, ramp_down])

                self.i_speed = 0
                u = np.array([1.0 / s for s in ramp_up]).sum()
                d = np.array([1.0 / s for s in ramp_down]).sum()
                f = np.array([1.0 / s for s in full]).sum()
                print(u, f, d, u + d + f)

            delta  = self.dest - self.curr

            if delta == 0:
                time.sleep(1 / Motor.MAX_SPEED)
                continue

            self.curr += self.dir
            self.speed = self.speeds[self.i_speed]
            self.i_speed += 1

            wp.digitalWrite(self.step_pin, 1)
            wp.digitalWrite(self.step_pin, 0)

            tosleep = (1.0 / self.speed) - (datetime.datetime.now() - self.last).seconds
            time.sleep(tosleep)
