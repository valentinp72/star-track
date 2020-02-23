import wiringpi as wp

class Mapping:

    wp.wiringPiSetup()

    ALTITUDE_DIR  = 24
    ALTITUDE_STEP = 25
    AZIMUTH_DIR   = 22
    AZIMUTH_STEP  = 23
    ENABLE = 21

    BUZZER    = 5
    EMERGENCY = 4
