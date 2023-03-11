import logging
import multiprocessing

from flask import jsonify

from enum import Enum, auto
from queue import Empty

logger = logging.getLogger(__name__)

pipe_flask, pipe_back = multiprocessing.Pipe(duplex=True)

class Command(Enum):

    # send/receive GPS location
    GET_gps_location = auto()
    SET_gps_location = auto()

    # ask the backend if it is running
    GET_status = auto()

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
            answer = pipe_flask.recv()
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
