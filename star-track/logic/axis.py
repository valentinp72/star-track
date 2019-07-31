
import time
import datetime
# import threading
import numpy as np
import wiringpi as wp

# from queue import Queue
from logic.mapping import Mapping

from multiprocessing import Process, Queue

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

class Motor(Process):

    DIR_FWD = 1
    DIR_BCK = -1

    ACCELERATION = 5  # 0.01
    MIN_SPEED    = 10  # 0.01
    MAX_SPEED    = 2000  # 0.0005

    def __init__(self, dir_pin, step_pin):
        super(Motor, self).__init__()
        self.dir_pin  = dir_pin
        self.step_pin = step_pin
        self.queue = Queue(1)

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
        while not self.queue.empty():
            self.queue.get()
        self.queue.put(dist)

    def set_dir(self, dir):
        if dir == 0:
            dir = Motor.DIR_FWD
        if dir != Motor.DIR_FWD and dir != Motor.DIR_BCK:
            raise ValueError("dir {} error".format(dir))
        if dir != self.dir:
            wp.digitalWrite(self.dir_pin, 0 if dir == Motor.DIR_FWD else 1)
            self.dir = int(dir)

    def get_speeds_dirs(
            self,
            current_speed, current_dir,
            target_speed, target_dir):

        valid_dir = [Motor.DIR_FWD, Motor.DIR_BCK]
        assert current_dir in valid_dir
        assert  target_dir in valid_dir

        current = current_dir * current_speed
        target  = target_dir  * target_speed

        steps = Motor.ACCELERATION
        if current > target:
            steps = -steps

        speeds_and_dirs = np.arange(current, target, steps)

        return speeds_and_dirs

    def get_speeds_dirs_to_position(self, wanted):
        delta = wanted - self.curr
        wanted_dir = Motor.DIR_FWD if delta > 0 else Motor.DIR_BCK

        if wanted_dir != self.dir:
            reverse = self.get_speeds_dirs(self.speed, self.dir, 0, wanted_dir)
            ramp_up = self.get_speeds_dirs(0, wanted_dir, Motor.MAX_SPEED, wanted_dir)
        else:
            reverse = np.array([])
            ramp_up = self.get_speeds_dirs(self.speed, self.dir, Motor.MAX_SPEED, wanted_dir)

        ramp_down = self.get_speeds_dirs(Motor.MAX_SPEED, wanted_dir, Motor.MIN_SPEED, wanted_dir)

        abd = abs(delta)# + len(reverse)
        if len(reverse) > 0:
            abd += len(reverse)

        total_duration      = len(ramp_up) + len(ramp_down)
        full_speed_duration = abd - total_duration
        print(full_speed_duration)
        full_speed = np.array([wanted_dir * Motor.MAX_SPEED for _ in range(full_speed_duration + len(reverse))])

        if full_speed_duration < 0:
            missing_steps = int((abd - total_duration) / 2)
            ramp_up   = ramp_up[:missing_steps]
            end_speed = abs(ramp_up[-1])
            ramp_down = self.get_speeds_dirs(end_speed, wanted_dir, Motor.MIN_SPEED, wanted_dir)
            if delta % 2 == 0:
                full_speed = [ramp_up[-1]]

        total_speeds = np.concatenate([reverse, ramp_up, full_speed, ramp_down])
        print([len(x) for x in [reverse, ramp_up, full_speed, ramp_down]])
        print(len(total_speeds))
        print(delta)
        print(abd, len(ramp_up) + len(full_speed) + len(ramp_down))
        print("")
    
        return total_speeds


    def run(self):
        while True:
            if not self.queue.empty():
                self.dest    = self.queue.get()
                self.speeds  = self.get_speeds_dirs_to_position(self.dest)
                self.i_speed = 0

            delta = self.dest - self.curr
            #print(delta)
            if self.i_speed >= len(self.speeds):
                if delta != 0:
                    print(delta)
                    exit()
                else:
                    time.sleep(1 / Motor.MAX_SPEED)
                continue

            speed_dir  = self.speeds[self.i_speed]
            self.speed = abs(speed_dir)
            self.set_dir(np.sign(speed_dir))
            self.curr += self.dir
            self.i_speed += 1

            if self.speed != 0:
                wp.digitalWrite(self.step_pin, 1)
                wp.digitalWrite(self.step_pin, 0)

                tosleep = (1.0 / self.speed)
                time.sleep(tosleep)
