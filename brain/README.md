# Brain source code

Current brain: Orange Pi Lite

## Client

```bash
cd client

# for development:
ionic serve

# for production:
ionic build
```

## Server

```bash
cd server
source env/bin/activate

# for development 
python3 server.py

# for production:
gunicorn -w 1 -b 0.0.0.0:5090 'server:app'
```
