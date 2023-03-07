import time
from queue import Empty

def main_loop(commands):
    while True:
        try:
            command = commands.get_nowait()
            print(command)
        except Empty:
            pass
        time.sleep(1)
