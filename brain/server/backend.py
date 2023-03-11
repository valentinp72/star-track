import time
import json
import logging

import filelock
from skyfield.api import (
    load,
    wgs84
)

import commands

logger = logging.getLogger(__name__)
lock = filelock.FileLock('/tmp/telescope_backend.lock')

class Backend:

    def __init__(self):
        self.toggle_state = False
        self.timescale = load.timescale()
        self.tracked = None

        self.planets = 'de440s.bsp'
        self.gps_location = {}
        self.weather = {}

        with open('data/objects.json', 'r') as f:
            self.objects_infos = json.load(f)

    def __main_loop__(self):
        """
        The main loop that runs in its own process and does everything.
        """
        while True:
            commands.Command.receive_handle_command(self)
            if self.running and self.tracked is not None:
                t = self.timescale.now()
                astrometric = self.current_pos.at(t).observe(self.tracked)
                alt, az, d = astrometric.apparent().altaz(
                    temperature_C=self._temp_C,
                    pressure_mbar=self._pressure_mbar
                )
                # returns https://rhodesmill.org/skyfield/api-units.html#skyfield.units.Angle
                print(f"Altitude: {alt}, Azimuth: {az}")
            else:
                time.sleep(self.sleep_delay)

    ########################################################################
    #           GETTERS AND SETTERS for COMMUNICATION WITH FLASK)          #
    ########################################################################

    @property
    def toggle_state(self):
        return self.running

    @toggle_state.setter
    def toggle_state(self, state):
        if not isinstance(state, bool):
            logger.error(
                f"toggle_state received invalid state={state}. Ignoring it."
            )
            return

        if hasattr(self, "running") and self.running == state:
            return
        self.running = state

        # adjust sleeping delay in the main loop (no need to have a
        # high-freq loop when not running skyfield computations)
        if self.running:
            self.sleep_delay = 0.01
            logger.info("Main telescope loop enabled.")
            if self.tracked is None:
                logger.warning(
                    'No tracking target currently selected, nothing will be ' \
                    'computed. You can call /track with the target.'
                )
        else:
            self.sleep_delay = 1
            logger.info("Main telescope loop is disabled.")

    @property
    def planets(self):
        out = []
        for code, object_names in self._planets.names().items():
            name = object_names[-1]
            object_name = name.replace('BARYCENTER', '').strip()
            try:
                additional_info = self.objects_infos[object_name]
            except KeyError:
                additional_info = {}
            out.append({
                'code': code,
                'name': name.title(),
                'object_name': object_name.title(),
                'additional_info': additional_info
            })
        return out

    @planets.setter
    def planets(self, ephemeris_name):
        self._planets = load(ephemeris_name)
        self.earth = self._planets['earth']

    @property
    def track_object(self):
        return self.tracked

    @track_object.setter
    def track_object(self, object_name):
        self.tracked = self._planets[object_name]
        logger.info(f'Current tracking target: {object_name}')

    @property
    def gps_location(self):
        return {
            'latitude': self._latitude,
            'longitude': self._longitude,
            'elevation': self._elevation,
        }

    @gps_location.setter
    def gps_location(self, loc):
        # default location if invalid data: Paris (France)
        self._latitude = loc.get('latitude',  48.864716)
        self._longitude = loc.get('longitude', 2.349014)
        self._elevation = loc.get('elevation', 35) # elevation in meters

        self.current_pos = self.earth + wgs84.latlon(
            self._latitude, self._longitude, elevation_m=self._elevation
        )

        logger.info(f"Location has changed to {self.gps_location}")

    @property
    def weather(self):
        # set the temperature and atmospheric pressure to adjust for 
        # atmospheric refraction
        return {
            'temperature_C': self._temp_C,
            'pressure_mbar': self._pressure_mbar
        }

    @weather.setter
    def weather(self, current_weather):
        # default fo of 10.0°C and 'standard' (computed using elevation)
        temp_C = current_weather.get('temperature_C', 10.0)
        pressure_mbar = current_weather.get('pressure_mbar', 'standard')

        if not -20 < temp_C < 60:
            logger.warning(
                f"Setting temperature to {temp_C} °C, but suspecting a " \
                f"temperature in °F."
            )
        if isinstance(pressure_mbar, float) and not 800 < pressure_mbar < 1200:
            logger.warning(
                f"Setting pressure to {pressure_mbar} mbar, but the unit " \
                "seems off."
            )
        elif pressure_mbar != 'standard':
            logger.error(
                f"Invalid pressure_mbar. Should be a float or `standard`. " \
                f"Got pressure_mbar={pressure_mbar} ({type(pressure_mbar)})"
            )

        self._temp_C = temp_C
        self._pressure_mbar = pressure_mbar

    ########################################################################
    #              METHODS FOR COMMUNICATION WITH FLASK                    #
    ########################################################################

    def handle_command(self, command, data):
        """
        This method is called when the backend receives a command.
        If it receives a GET command (.is_getter == True), we call the
        corresponding property on the Backend object, and send it back to Flask.
        If it receives a SET command, we just call the set method on the object.
        """
        if command.is_getter:
            property_name = command.property_name
            info = getattr(self, property_name)      # retreiving the info
            commands.pipe_back.send((command, info)) # sending it back to flask
        elif command.is_setter:
            property_name = command.property_name
            setattr(self, property_name, data)
        else:
            raise NotImplementedError(f"Unkown command type for {command}.")

    @classmethod
    def start_backend(cls, pipe_back):
        """
        This is the method invoked when creating the special process for the
        backend. We setup a file lock to prevent multiple backend from running
        simultaneously, however this won't prevent the webserver from trying to
        reach other backends if they have been launched.
        """
        try:
            with lock.acquire(timeout=5):
                commands.pipe_back = pipe_back
                logger.info("Creating the backend and starting it.")
                backend = cls()
                backend.__main_loop__()
        except filelock.Timeout:
            logger.error(
                f"Error when starting backend. Could not acquire the lock " \
                f"{lock.lock_file}. Either the backend is already running " \
                f"in another process, or the file lock is dangling. " \
                f"Please make sure only ONE backend process is launched."
            )
