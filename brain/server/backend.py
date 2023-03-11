import time
import logging

import filelock
from skyfield.api import load

import commands

logger = logging.getLogger(__name__)
lock = filelock.FileLock('/tmp/telescope_backend.lock')

class Backend:

    STATUS_RUNNING = "running"
    STATUS_STOPPED = "stopped"

    def __init__(self):
        self.status = Backend.STATUS_RUNNING
        self.gps_location = {}
        self.sleep_delay = 0.01

        self.timescale = load.timescale()
        self.lanets = load('de405.bsp')

    def __main_loop__(self):
        while True:
            commands.Command.receive_handle_command(self)
            time.sleep(self.sleep_delay)

    @property
    def gps_location(self):
        return {
            'latitude': self._latitude,
            'longitude': self._longitude
        }

    @gps_location.setter
    def gps_location(self, loc):
        # default location if invalid data: Paris (France)
        self._latitude = loc.get('latitude',  48.864716)
        self._longitude = loc.get('longitude', 2.349014)
        logger.info(f"Location has changed to {self.gps_location}")
        # TODO: notify the backend that the GPS location has changed

    def handle_command(self, command, data):
        """
        This method is called when the backend receives a command.
        If it receives a GET command (.is_getter == True), we call the
        corresponding property on the Backend object, and send it back to Flask.
        If it receives a SET command, we just call the set method on the object.
        """
        if command.is_getter:
            property_name = command.property_name
            info = getattr(self, property_name) # retreiving the information
            commands.pipe_back.send(info)       # sending it back to flask
        elif command.is_setter:
            property_name = command.property_name
            setattr(self, property_name, data)
        else:
            raise NotImplementedError(f"Unkown command type for {command}.")

    @classmethod
    def start_backend(cls, pipe_back):
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
