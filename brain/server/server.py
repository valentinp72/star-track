import time
from flask import Flask
from celery import Celery

app = Flask(__name__)
app.config['CELERY_BROKER_URL'] = 'redis://localhost:6379/0'
app.config['CELERY_RESULT_BACKEND'] = 'redis://localhost:6379/0'

celery = Celery(app.name, broker=app.config['CELERY_BROKER_URL'])
celery.conf.update(app.config)

@celery.task
def add_together(a, b):
    return a + b

@app.route("/")
def root():
    return {
        "message": "Welcome to the API for the star-track server."
    }


@celery.task
def heavy_func():
    for i in range(10):
        time.sleep(1)
        print(i)
    # while True:
    #     pass

heavy_func()
