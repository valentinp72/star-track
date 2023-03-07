import os
import time
from flask import Flask
from queue import Queue, Empty
import multiprocessing

################################################################################

commands = multiprocessing.Queue()
app = Flask(__name__)

################################################################################

def main_loop(commands):
    i = 0
    while True:
        try:
            command = commands.get_nowait()
            print(command)
        except Empty:
            pass
        print(i)
        i += 1
        time.sleep(1)

@app.before_first_request
def setup_main_process():
    main_process = multiprocessing.Process(
        target=main_loop,
        args=(commands,),
        daemon=True
    )
    main_process.start()

################################################################################

@app.route('/alive')
def alive():
    return {
        'message': "I'm alive!"
    }, 200

@app.route('/')
def root():
    commands.put_nowait({'action': 'something'})
    return {
        'message': 'Welcome to the API for the star-track server.'
    }, 200

################################################################################

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port='5090', use_reloader=False)
