// SPDX-License-Identifier: Apache-2.0
import {build} from 'esbuild';
import {mkdir, readFile, readdir, rm, writeFile} from 'node:fs/promises';
import {createRequire} from 'node:module';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {verificationPlugin} from './integration.mjs';

const root = path.dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
const entry = require.resolve('esp-web-tools/dist/install-button.js');
const upstreamRequire = createRequire(entry);
const loaderPath = upstreamRequire.resolve('esptool-js');
const loaderPackage = JSON.parse(await readFile(path.resolve(loaderPath, '../../package.json'), 'utf8'));
if (loaderPackage.version !== '0.7.0') throw new Error('The installer must resolve esptool-js 0.7.0');
const output = process.argv[2] ? path.resolve(process.argv[2]) : path.resolve(root, '../../docs/flasher-vendor');
await rm(output, {recursive: true, force: true});
await mkdir(output, {recursive: true});
const result = await build({
  entryPoints: {'install-button': entry},
  outdir: output,
  bundle: true,
  splitting: true,
  format: 'esm',
  platform: 'browser',
  target: 'es2022',
  minify: true,
  legalComments: 'linked',
  metafile: true,
  plugins: [verificationPlugin],
  banner: {js: '/* Surveillance Hound verified installer. ESP Web Tools integration modified for download and device verification. See LICENSES.txt. */'}
});

// Archive full license/notice texts for each package actually included.
const packages = new Set();
for (const input of Object.keys(result.metafile.inputs)) {
  let dir = path.dirname(path.resolve(input));
  while (dir.includes('node_modules')) {
    try {
      await readFile(path.join(dir, 'package.json'));
      packages.add(dir);
      break;
    } catch { dir = path.dirname(dir); }
  }
}
let notices = 'Surveillance Hound browser installer\nCopyright 2026 Jascha Wanger (https://dx.pe)\nIntegration: Apache-2.0. ESP Web Tools is modified to verify downloads and flash contents.\n\n';
for (const dir of [...packages].sort()) {
  const pkg = JSON.parse(await readFile(path.join(dir, 'package.json'), 'utf8'));
  const files = (await readdir(dir)).filter(name => /^(licen[cs]e|copying|notice)([.-]|$)/i.test(name));
  if (!files.length) throw new Error(`Missing license text for ${pkg.name}`);
  notices += `=== ${pkg.name} ${pkg.version} (${pkg.license}) ===\n`;
  for (const file of files.sort()) notices += `${file}\n${await readFile(path.join(dir, file), 'utf8')}\n`;
}
await writeFile(path.join(output, 'LICENSES.txt'), notices);
console.log(`Built verified ESP Web Tools 10.4.0 / esptool-js 0.7.0 installer in ${output}`);
