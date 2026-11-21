#!/usr/bin/env bash
# Makes the LittleFS image with the phone app, and writes it to the mount.
#
#   ./app-fs.sh                  build/app-fs.bin from app/dist
#   ./app-fs.sh /dev/ttyUSB0     the same, then writes it to the board on that port
#
# Also makes build/app-fs.ota for updates over Wi-Fi (ota.sh): only the image's
# blocks that are not blank, each after its number. LittleFS never reads a block
# it has not written, so the others can keep old data.
#
# Build the app first (in app/: npm run build). Text files are stored gzipped
# (name.gz), which the web server sends as they are; pictures stay as they are.
# The offset and size are those of the spiffs partition in partitions.csv.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
dist="$here/../../app/dist"
out="$here/build"
offset=0x610000
size=0x9E0000

tools="$HOME/.arduino15/packages/esp32/tools"
mklittlefs=$(ls -d "$tools"/mklittlefs/*/mklittlefs | sort -V | tail -1)
esptool=$(ls -d "$tools"/esptool_py/*/esptool | sort -V | tail -1)

[ -f "$dist/index.html" ] || { echo "no app build in $dist: run npm run build in app/" >&2; exit 1; }

rm -rf "$out/app-fs"
mkdir -p "$out/app-fs"
cp -r "$dist"/. "$out/app-fs/"
find "$out/app-fs" -type f \( -name '*.html' -o -name '*.js' -o -name '*.css' -o -name '*.json' \
  -o -name '*.webmanifest' -o -name '*.svg' -o -name '*.txt' \) -exec gzip -9 -n {} +

"$mklittlefs" -c "$out/app-fs" -b 4096 -p 256 -s "$size" "$out/app-fs.bin" >/dev/null
echo "$out/app-fs.bin: $(du -sh --apparent-size "$out/app-fs" | cut -f1) of files in a $((size / 1024)) KB image"

# app-fs.ota: "STFS", version 1, the number of blocks (little-endian 32-bit
# integers), then for each block its number and its 4096 bytes.
python3 - "$out/app-fs.bin" "$out/app-fs.ota" <<'PY'
import struct, sys
data = open(sys.argv[1], 'rb').read()
B = 4096
blank = b'\xff' * B
blocks = [i for i in range(len(data) // B) if data[i * B:(i + 1) * B] != blank]
with open(sys.argv[2], 'wb') as f:
    f.write(b'STFS' + struct.pack('<II', 1, len(blocks)))
    for i in blocks:
        f.write(struct.pack('<I', i) + data[i * B:(i + 1) * B])
print(f'{sys.argv[2]}: {len(blocks)} blocks of {len(data) // B}')
PY

if [ $# -ge 1 ]; then
  "$esptool" --chip esp32s3 --port "$1" --baud 921600 write-flash "$offset" "$out/app-fs.bin"
fi
