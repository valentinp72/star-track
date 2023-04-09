# Brain source code

Current brain: Orange Pi Lite

## Installation

Make sure the environment variable `STAR_TRACK` is exported in the `.bashrc`/`.zshrc` file:
```bash
export STAR_TRACK="/path/to/the/star-track/git/repo"
```

### Installing packages

```bash
# client
apt-get install nodejs npm
npm install -g @ionic/cli
npm install $STAR_TRACK/brain/client

# server
apt-get install python3 python3-venv
python3 -m venv $STAR_TRACK/brain/server/env
source $STAR_TRACK/brain/server/env/bin/activate
pip3 install -r $STAR_TRACK/brain/server/requirements.txt
```

### Preparing Nginx

```bash
apt-get install nginx
```

You need to copy/link the file `config/nginx_star_track` to `/etc/nginx/sites-enabled`.
You might have some edits to do in it (paths for example).

### Optional: (Re)building the icons and splashscreens:

Icons and XCode splash screen:
```bash
npx @capacitor/assets generate
```

PWA splash screen
You need a chromium browser installed, not compatible with CLI systems.
```bash
npx pwa-asset-generator \
	$STAR_TRACK/brain/client/public/assets/telescope-light.svg \
	$STAR_TRACK/brain/client/public/assets/icons \
	--background "linear-gradient(0deg, rgba(88,86,214,1) 0%, rgba(198,68,252,1) 100%)" \
	--splash-only --portrait-only \
	--type png \
	--manifest $STAR_TRACK/brain/client/dist/manifest.json
```
-->

### Creating SSL certificates

We create a self-signed CA authority with a site certificate with. On iOS, for
icons and other images to load, you have to download the root CA and to
"Enable full trust for root certificates" in "Settings > General > About > 
Certificate Trust Settings". The Root CA will be available for download
in the settings page of the web app.

```bash
# creating a certificate authority (CA)
apt-get install mkcert
mkcert -install
cp "`mkcert -CAROOT`/rootCA.pem" $STAR_TRACK/brain/client/public/assets/rootCA.pem

# creating the website certificate 
hostname=`hostname`
mkcert \
	-key-file $STAR_TRACK/brain/ssl/certificate.key \
	-cert-file $STAR_TRACK/brain/ssl/certificate.crt \
	$hostname $hostname.local localhost 127.0.0.1 ::1
```

## Utilisation

### Client

```bash
cd $STAR_TRACK/brain/client

# for development:
ionic serve
```

To build the app with Xcode and sideload it:

```bash
ionic capacitor copy ios
ionic capacitor update
ionic capacitor open ios
```
> (stolen from [stackoverflow](https://stackoverflow.com/a/36449958) and [reddit](https://www.reddit.com/r/Xcode/comments/kc3qu4/comment/gfrv8gz/?utm_source=share&utm_medium=web2x&context=3))
> 1. Archive your project as a generic iOS device
> 2. Right click the archive and click “Find in Finder”
> 3. Explore the contents of the archive until you find the .app
> 4. Now, create directory and name it as Payload, copy .app into Payload directory.
> 5. Archive/Compress(.zip) this Payload directory, rename file extension from .zip to .ipa


## Server

```bash
cd $STAR_TRACK/brain/server
source env/bin/activate

# for development 
python3 server.py

# for production:
gunicorn -w 1 -b 0.0.0.0:5090 'server:app'
```
