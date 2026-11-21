#!/usr/bin/env bash
# Updates the mount over its Wi-Fi, from a computer joined to its network.
#
#   ./ota.sh firmware [HOST]    compiles the firmware and sends it
#   ./ota.sh app [HOST]         makes the app's LittleFS image and sends it
#
# HOST defaults to 192.168.4.1. The mount must be idle, parked or in E-stop; it
# restarts when the update is written. Park it first to keep the alignment.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
what=${1:-}
host=${2:-192.168.4.1}

case "$what" in
  firmware)
    arduino-cli compile --output-dir "$here/build/firmware" "$here"
    file="$here/build/firmware/star-track.ino.bin"
    url="http://$host/update"
    ;;
  app)
    "$here/app-fs.sh"
    file="$here/build/app-fs.ota"
    url="http://$host/update?target=app"
    ;;
  *)
    echo "usage: $0 firmware|app [HOST]" >&2
    exit 2
    ;;
esac

echo "sending $(du -h --apparent-size "$file" | cut -f1) to $url"
curl --fail-with-body --progress-bar -F "file=@$file" "$url"
