# Brain source code

Current brain: Orange Pi Lite

## Creating SSL certificates

We create a self-signed CA authority with a site certificate with. On iOS, for
icons and other images to load, you have to download the root CA and to
"Enable full trust for root certificates" in "Settings > General > About > 
Certificate Trust Settings". The Root CA will be available for download
in the settings page of the web app.

```bash
# creating a certificate authority (CA)
mkcert -install
ca_root="`mkcert -CAROOT`/rootCA.pem"

# creating the 
hostname=`hostname`
mkcert \
	-key-file ssl/$hostname.key \
	-cert-file ssl/$hostname.crt \
	$hostname $hostname.local localhost 127.0.0.1 ::1

cp "$ca_root" client/public/assets/rootCA.pem
```

## Client

```bash
cd client

# optional: to (re)build the icons and splashscreens:
npx pwa-asset-generator \
	public/assets/telescope-light.svg \
	public/assets/icon \
	--background "linear-gradient(0deg, rgba(88,86,214,1) 0%, rgba(198,68,252,1) 100%)" \
	--mstile \
	--favicon \
	--type png \
	--manifest dist/manifest.json

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
