import os
import logging
import multiprocessing

from flask import (
    Flask,
    request,
    abort,
    jsonify
)

import commands
from commands import Command
from backend import Backend

logging.basicConfig(
    format="%(asctime)s | %(levelname)s | %(name)s | %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=os.environ.get("LOGLEVEL", "INFO").upper(),
)
logger = logging.getLogger(__name__)

################################################################################

app = Flask(__name__)

@app.before_first_request
def setup_main_process():
    # setting up the main backend process that computes everything and talks
    # with the arduino
    main_process = multiprocessing.Process(
        target=Backend.start_backend,
        args=(commands.pipe_back,),
        daemon=True
    )
    main_process.start()

################################################################################

@app.route('/toggle', methods=['GET', 'POST'])
def toggle():
    """
    Toggle ('on' or 'off') the telescope tracking loop.
    Arguments to the POST method:
        {
            'state': true / false
        }
    """
    if request.method == 'GET':
        return Command.GET_toggle_state()
    elif request.method == 'POST':
        data = request.json
        toggle_state = data.get('state')

        if toggle_state is None:
            abort(400)
        if not isinstance(toggle_state, bool):
            abort(400)

        return Command.SET_toggle_state(toggle_state)

@app.route('/gps', methods=['GET', 'POST'])
def gps():
    """
    Get the current configured GPS location, or update it.
    Locations are and should be given in the Decimal Degrees (DD) format.
    Arguments to the POST method:
        {
            'latitude': latitude in float format,
            'longitude' longitude in float format
        }
    """
    if request.method == 'GET':
        return Command.GET_gps_location()
    elif request.method == 'POST':
        data = request.json
        latitude = data.get('latitude')
        longitude = data.get('longitude')

        if latitude is None or longitude is None:
            abort(400)

        return Command.SET_gps_location({
            "latitude": latitude,
            "longitude": longitude,
        })
    else:
        abort(405)

@app.route('/planets', methods=['GET', 'POST'])
def planets():
    """
    - Get all the available planets that can be tracked.
    - Set the ephemeris file (.bsp) for the loading the planets.
    Arguments to the POST method:
        {
            'ephemeris_name': 'de440s.bsp'
        }
    """
    if request.method == 'GET':
        return Command.GET_planets()
    elif request.method == 'POST':
        data = request.json
        ephemeris_name = data.get('ephemeris_name')

        if ephemeris_name is None:
            abort(400)
        return Command.SET_planets(ephemeris_name)

@app.route('/track', methods=['POST'])
def track():
    """
    Track the specified object.
    Arguments to the POST method:
        {
            'target': name or code for the target
        }
    """
    data = request.json
    target = data.get('target')

    if target is None:
        abort(400)
    return Command.SET_track_object(target)

@app.route('/')
def root():
    """
    Say hello to the API and get the available endpoints.
    """
    routes = {}
    for r in app.url_map._rules:
        routes[r.rule] = {}
        routes[r.rule]["function"] = r.endpoint
        routes[r.rule]["methods"] = list(r.methods)
        doc = app.view_functions[r.endpoint].__doc__
        if doc is not None:
            routes[r.rule]["documentation"] = " ".join(doc.split())
    routes.pop("/static/<path:filename>")
    return {
        'message': 'Welcome to the API for the star-track server.',
        'methods': routes
    }

################################################################################

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port='5090')
    # app.run(debug=True, host='0.0.0.0', port='5090', use_reloader=False)
