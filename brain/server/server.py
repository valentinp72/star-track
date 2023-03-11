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
    - GET the current telescope tracking status.
    - SET the toggle of telescope tracking loop.
        Arguments:
            {
                'state': true / false
            }
    If `true`, the tracking loop is running, `false` otherwise. Note that the
    loop will run only if toggle=True AND that a track target has been
    configured (see /track).
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
    - GET the current configured GPS location.
    - SET the current GPS location.
        Arguments:
            {
                'latitude': DD latitude (float),
                'longitude' DD longitude (float),
                'elevation': elevation above sea lever (float, optional)
            }
    Locations are and should be given in the Decimal Degrees (DD) format.
    """
    if request.method == 'GET':
        return Command.GET_gps_location()
    elif request.method == 'POST':
        data = request.json
        latitude = data.get('latitude')
        longitude = data.get('longitude')
        elevation = data.get('elevation')

        if latitude is None or longitude is None:
            abort(400)

        return Command.SET_gps_location({
            "latitude": latitude,
            "longitude": longitude,
            "elevation": elevation,
        })
    else:
        abort(405)

@app.route('/weather', methods=['GET', 'POST'])
def weather():
    """
    - GET the current configured temperature (in °C) and atmospheric pressure
       used in the system (in mbar).
    - SET the temperature (in °C) and atmospheric pressure (in mbar).
        Arguments:
            {
                'temperature_C': temp (float),
                'pressure_mbar': pressure (float, optional)
            }
    The temperature and pressure are used to adjust the target position
    according to the atmospheric refraction. If the pressure is not specified
    (by default), then the refraction will be computed using the elevation
    specified in the GPS coordinates and an approximation of the pressure at
    that elevation.
    """
    if request.method == 'GET':
        return Command.GET_weather()
    elif request.method == 'POST':
        data = request.json
        temperature_C = data.get('temperature_C')
        pressure_mbar = data.get('pressure_mbar')

        if temperature_C is None:
            abort(400)

        return Command.SET_weather({
            "temperature_C": temperature_C,
            "pressure_mbar": pressure_mbar
        })

@app.route('/planets', methods=['GET', 'POST'])
def planets():
    """
    - GET all the available planets that can be tracked.
    - SET the ephemeris file (.bsp) for the loading the planets.
        Arguments:
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
    - SET: Ask the server to track the specified object.
        Arguments:
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
    out = """
        <h1>Star-Track Telescope REST API</h1>
        Welcome to the REST API of the star-track server.
        <hr>
        <ul>
    """
    for r in app.url_map._rules:
        doc = app.view_functions[r.endpoint].__doc__ or "Not documented."
        methods = " | ".join(sorted(list(r.methods)))
        out += f"""
            <li>
                <font color='red'>{r.rule}</font> -
                {methods}
                <pre>{doc}</pre>
            </li>
        """
    out += "</ul>"
    out += "<hr>\n"
    return out

################################################################################

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port='5090')
    # app.run(debug=True, host='0.0.0.0', port='5090', use_reloader=False)
