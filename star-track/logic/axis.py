
import time
import threading
import wiringpi as wp

from Queue import Queue
from mapping import Mapping


class Axis:

    def alitude():
        return Axis(Mapping.ALTITUDE_DIR, Mapping.ALTITUDE_STEP)

    def azimuth():
        return Axis(Mapping.AZIMUTH_DIR, Mapping.AZIMUTH_STEP)

    #########

    def __init__(self, dir_pin, step_pin):
        self.motor = Motor(dir_pin, step_pin)

        self.current = 0

    def set_position(self, steps):
        pass

class Motor(threading.Thread):

    MICROSTEPPING = 16

    def __init__(self, dir_pin, step_pin):
        self.dir_pin  = dir_pin
        self.step_pin = step_pin
        self.queue = Queue(1)

        # current and destination positions
        self.curr = 0
        self.dest = 0

        wp.pinMode(self.dir_pin,  wp.OUTPUT)
        wp.pinMode(self.step_pin, wp.OUTPUT)

    def set_dist(self, dist):
        if not self.queue.empty():
            self.queue.get()
        self.queue.put(dist)

    def run(self):
        while True:
            if not self.queue.empty():
                self.dest = self.queue.get()

            # goto self.dest
            wp.digitalWrite(self.step_pin, 1)
            wp.digitalWrite(self.step_pin, 0)
            time.sleep(1)
