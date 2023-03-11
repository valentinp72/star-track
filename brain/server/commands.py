import logging
import multiprocessing

from flask import (
    jsonify, abort
)

from enum import Enum, auto
from queue import Empty

logger = logging.getLogger(__name__)

pipe_flask, pipe_back = multiprocessing.Pipe(duplex=True)

class Command(Enum):

    # send/receive GPS location
    GET_gps_location = auto()
    SET_gps_location = auto()

    # toggle on or off the main telescope tracking loop
    GET_toggle_state = auto()
    SET_toggle_state = auto()

    # retreive the available planets
    GET_planets = auto()
    SET_planets = auto()

    # set the target object to track
    SET_track_object = auto()

    @property
    def is_getter(self):
        return self.name.startswith('GET_')

    @property
    def is_setter(self):
        return self.name.startswith('SET_')

    @property
    def property_name(self):
        if self.is_getter or self.is_setter:
            return self.name[4:]
        else:
            raise ValueError(f"{self} doesn't have a property.")

    def __call__(self, data=None):
        """
        To be called only on Flask. Will send the command with the associated
        data to the backend. If we sent a GET command (is_getter == True) then
        we wait for the answer.
        WARNING: this procedue will break if the backend sends messages without
        being asked to. This works only when the backend replies to each GET
        and only to that.
        """
        pipe_flask.send((self, data))
        if self.is_getter:
            # to keep the message synced, we depop all messages until we
            # have received the correct one. This might happen due to the 15s
            # timeout: abort is called so the message will be received on the
            # next Flask API call.
            command = None
            while command != self:
                if pipe_flask.poll(timeout=15):
                    command, answer = pipe_flask.recv()
                    if command != self:
                        logger.warning(
                            f"Received {command} but waiting for {self}. " \
                            f"Ignoring this message to keep up the sync."
                        )
                else:
                    logger.warning(
                        f"Abort of connection waiting from the backend after " \
                        f"15s for command={command}."
                    )
                    abort(504)
            else:
                return jsonify(answer)
        else:
            return {
                'message': 'Sent to backend',
                'data': data
            }, 202

    @classmethod
    def receive_handle_command(cls, backend):
        """
        To be called only on the Backend. If a message from Flask is available,
        call the handle method on the backend with this message.
        """
        if pipe_back.poll():
            received = pipe_back.recv()
            backend.handle_command(*received)
