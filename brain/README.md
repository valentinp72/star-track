# Brain source code

Current brain: Orange Pi Lite

## Client

```bash
cd client

# for debug:
ionic serve

# for production:
ionic build
```

## Server

```bash
cd server
source env/bin/activate

# for debug:
flask --app server --debug run --host=0.0.0.0 --port=5090

# for production:
gunicorn -w 1 -b 0.0.0.0:5090 'server:app'
```
