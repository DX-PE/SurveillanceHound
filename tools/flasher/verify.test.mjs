// SPDX-License-Identifier: Apache-2.0
import assert from 'node:assert/strict';
import {createHash, webcrypto} from 'node:crypto';
import {createRequire} from 'node:module';
import test from 'node:test';
import {build} from 'esbuild';
import {verifyFirmware, writeAndVerify} from './verify.mjs';
import {verificationPlugin} from './integration.mjs';

globalThis.crypto ??= webcrypto;
const data = Uint8Array.from({length: 257}, (_, i) => i % 256);
const sha256 = bytes => createHash('sha256').update(bytes).digest('hex');
const md5 = bytes => createHash('md5').update(bytes).digest('hex');
const part = {path: 'factory.bin', offset: 0, size: data.length, sha256: sha256(data)};
const options = {fileArray: [{data, address: 0}], flashMode: 'keep', flashSize: 'keep', flashFreq: 'keep'};

test('download verification rejects truncation, corruption and missing pins before writes', async () => {
  await verifyFirmware(data, part);
  for (const [bytes, metadata] of [
    [data.slice(1), part], [data.map(b => b ^ 1), part],
    [data, {...part, sha256: undefined}], [data, {...part, size: undefined}]
  ]) await assert.rejects(verifyFirmware(bytes, metadata), /Firmware/);
});

test('device check covers the complete byte range and honors typed-array boundaries', async () => {
  const calls = [];
  const view = Uint8Array.from([99, ...data, 88]).subarray(1, data.length + 1);
  const loader = {
    async writeFlash() { calls.push('write'); },
    async flashMd5sum(address, size) { calls.push([address, size]); return md5(data).toUpperCase(); }
  };
  await writeAndVerify(loader, {...options, fileArray: [{data: view, address: 0x1000}]});
  assert.deepEqual(calls, ['write', [0x1000, data.length]]);
});

test('successful write progress cannot bypass a bad, missing or failed device checksum', async () => {
  for (const actual of [md5(data.slice(0, 100)), '', undefined]) {
    await assert.rejects(writeAndVerify({async writeFlash() {}, async flashMd5sum() { return actual; }}, options), /verification failed/);
  }
  await assert.rejects(writeAndVerify({
    async writeFlash() {}, async flashMd5sum() { throw new Error('Serial disconnected'); }
  }, options), /Serial disconnected/);
});

// Exercise the actual patched upstream flash state machine. Only serial I/O,
// downloads and reset delays are simulated; success/error sequencing is real.
const require = createRequire(import.meta.url);
const upstream = await build({
  entryPoints: [require.resolve('esp-web-tools/dist/flash.js')],
  bundle: true, write: false, platform: 'node', format: 'esm',
  plugins: [verificationPlugin, {
    name: 'fake-serial-only', setup(builder) {
      builder.onResolve({filter: /^esptool-js$/}, () => ({path: 'serial', namespace: 'fake'}));
      builder.onLoad({filter: /.*/, namespace: 'fake'}, () => ({contents: `
        export class Transport {
          async setRTS() { globalThis.serialCalls.push('reset'); }
          async disconnect() { globalThis.serialCalls.push('disconnect'); }
        }
        export class ESPLoader {
          chip = {CHIP_NAME: 'ESP32'};
          async main() {}
          async flashId() {}
          async eraseFlash() { globalThis.serialCalls.push('erase'); }
          async writeFlash() { globalThis.serialCalls.push('write'); }
          async flashMd5sum(address, size) {
            globalThis.serialCalls.push(['verify', address, size]);
            if (globalThis.deviceDigest instanceof Error) throw globalThis.deviceDigest;
            return globalThis.deviceDigest;
          }
          async after() {}
        }`, loader: 'js'}));
      builder.onLoad({filter: /esp-web-tools\/dist\/util\/sleep\.js$/}, () => ({contents: 'export const sleep = async () => {};', loader: 'js'}));
    }
  }]
});
const {flash} = await import(`data:text/javascript;base64,${Buffer.from(upstream.outputFiles[0].text).toString('base64')}`);

test('upstream reports FINISHED only after a matching full-image device checksum', async () => {
  for (const scenario of ['ok', 'incomplete', 'disconnected', 'bad-download']) {
    globalThis.window = {};
    globalThis.location = {toString: () => 'https://example.invalid/flash/'};
    globalThis.serialCalls = [];
    globalThis.deviceDigest = scenario === 'disconnected' ? new Error('Serial disconnected') :
      scenario === 'incomplete' ? md5(data.slice(0, 100)) : md5(data);
    const downloaded = scenario === 'bad-download' ? data.map(b => b ^ 1) : data;
    globalThis.fetch = async () => ({ok: true, blob: async () => new Blob([downloaded])});
    globalThis.FileReader = class {
      addEventListener(_, callback) { this.callback = callback; }
      async readAsArrayBuffer(blob) { this.result = await blob.arrayBuffer(); this.callback(); }
    };
    const states = [];
    await flash(event => { states.push(event); serialCalls.push(event.state); }, {getInfo: () => ({})},
      '../firmware/manifest.json', {builds: [{chipFamily: 'ESP32', parts: [part]}]}, true);
    const finished = states.some(event => event.state === 'finished');
    assert.equal(finished, scenario === 'ok', scenario);
    assert.equal(serialCalls.includes('write'), scenario !== 'bad-download', scenario);
    assert.equal(serialCalls.includes('erase'), scenario !== 'bad-download', scenario);
    assert.ok(serialCalls.includes('disconnect'));
    if (scenario === 'ok') {
      assert.ok(serialCalls.findIndex(call => Array.isArray(call)) < serialCalls.indexOf('finished'));
      assert.deepEqual(serialCalls.find(call => Array.isArray(call)), ['verify', 0, data.length]);
    } else {
      assert.equal(states.at(-1).state, 'error');
      assert.equal(states.at(-1).details.error, scenario === 'bad-download' ? 'failed_firmware_download' : 'write_failed');
    }
  }
});
