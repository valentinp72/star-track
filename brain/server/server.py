import os
import time
from flask import Flask
from queue import Queue, Empty
from threading import Thread

commands = Queue()
app = Flask(__name__)

def main_loop():
    # https://stackoverflow.com/a/69869088
    while True:
        try:
            command = commands.get_nowait()
            print(command)
        except Empty:
            pass
        time.sleep(1)

if not app.debug or os.environ.get('WERKZEUG_RUN_MAIN') == 'true':
    # starting the main background process only once, even if we are in the
    # debugging mode which can potentially reload the code
    # TODO: reloading should kill the old thread and start a new one
    Thread(target=main_loop, daemon=True).start()

@app.route("/")
def root():
    commands.put_nowait({ 'action': 'something' })
    return {
        "message": "Welcome to the API for the star-track server."
    }

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port='5090', use_reloader=False)
