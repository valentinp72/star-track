import time

from logic.axis import Axis

e = Axis.azimuth()
#e.motor.set_dist(209)
#e.move_angle(degrees=-90)

#time.sleep(1)

#e.move_angle(seconds=340)


from skyfield.api import load
from skyfield.api import Topos


ts = load.timescale()

planets = load('de421.bsp')
earth = planets["earth"]
moon = planets["moon"]
jupiter = planets["jupiter barycenter"]

stations_url = 'http://celestrak.com/NORAD/elements/stations.txt'
satellites = load.tle(stations_url)
satellite = satellites['ISS (ZARYA)']

target = jupiter 
#target = earth + satellite
print(target)
here = earth + Topos('47.827435 N', '-0.397186 W')

while True:

    t = ts.now()
    astrometric = here.at(t).observe(target)
    alt, az, d = astrometric.apparent().altaz()

    # print(alt, az)
    d, m, s = az.dms(warn=False)
    d, m, s = int(d), int(m), int(s)
    #d, m, s = 90, 0, 0
    print(d, m, s)
    e.move_angle(degrees=d, minutes=m, seconds=s)

    time.sleep(1)
