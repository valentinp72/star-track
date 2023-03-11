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
from backend import main_loop

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
        target=main_loop,
        args=(commands.pipe_back,),
        daemon=True
    )
    main_process.start()

################################################################################

@app.route('/status')
def status():
    """
    Return if the backend is 'running' or 'stopped.'
    """
    return commands.Command.GET_status()

@app.route('/gps', methods=['GET', 'POST'])
def gps():
    """
    Get the current configured GPS location, or update it.
    Locations are and should be given in the Decimal Degrees (DD) format.
    """
    if request.method == 'GET':
        return commands.Command.GET_gps_location()
    elif request.method == 'POST':
        data = request.json
        latitude = data.get('latitude')
        longitude = data.get('longitude')

        if latitude is None or longitude is None:
            abort(400)

        return commands.Command.SET_gps_location({
            "latitude": latitude,
            "longitude": longitude,
        })
    else:
        abort(405)

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
    print(routes)
    return {
        'message': 'Welcome to the API for the star-track server.',
        'methods': routes
    }

################################################################################

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port='5090', use_reloader=False)
