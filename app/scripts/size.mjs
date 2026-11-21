// The build's size as the mount will store and serve it: every file in dist/
// gzipped. Run after `vite build` (npm run size does both).
//
// Budget: 512 KB gzipped, a fraction of the ESP32-S3's flash, so that the app
// loads in a few seconds over the mount's Wi-Fi.

import { readdirSync, readFileSync, statSync } from 'node:fs';
import { join, relative } from 'node:path';
import { gzipSync } from 'node:zlib';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const BUDGET = 512 * 1024;
const dist = join(dirname(fileURLToPath(import.meta.url)), '..', 'dist');

function files(dir) {
  return readdirSync(dir).flatMap((f) => {
    const p = join(dir, f);
    return statSync(p).isDirectory() ? files(p) : [p];
  });
}

let raw = 0;
let gz = 0;
const rows = files(dist).map((p) => {
  const data = readFileSync(p);
  const z = gzipSync(data, { level: 9 }).length;
  raw += data.length;
  gz += z;
  return [relative(dist, p), data.length, z];
});
const kb = (n) => `${(n / 1024).toFixed(1)} KB`.padStart(10);
for (const [name, r, z] of rows.sort((a, b) => b[2] - a[2])) console.log(`${kb(r)} ${kb(z)}  ${name}`);
console.log(`${kb(raw)} ${kb(gz)}  total (raw, gzipped); budget ${BUDGET / 1024} KB gzipped`);
if (gz > BUDGET) {
  console.error('Over budget.');
  process.exit(1);
}
