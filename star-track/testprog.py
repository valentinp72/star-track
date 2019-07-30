import time
import wiringpi as wp
from logic.axis import Axis

wp.wiringPiSetup()

e = Axis.azimuth()
#e.motor.set_dist(209)
e.move_angle(degrees=360)

time.sleep(2)

#e.move_angle(degrees=-10)
