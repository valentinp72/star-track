from flask import Flask
import multiprocessing

from backend import main_loop

################################################################################

commands = multiprocessing.Queue()
app = Flask(__name__)

@app.before_first_request
def setup_main_process():
    # setting up the main backend process that computes everything and talks
    # with the arduino
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
