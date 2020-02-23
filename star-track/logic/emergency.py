import wiringpi as wp

from logic.mapping import Mapping

wp.pinMode(Mapping.EMERGENCY, wp.INPUT)

class Emergency:

    def allowed_move():
        return wp.digitalRead(Mapping.EMERGENCY) == 0
