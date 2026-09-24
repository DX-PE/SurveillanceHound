// SPDX-License-Identifier: Apache-2.0
// Replay identical user actions through native and Wasm adapters; compare every pixel.
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {once} from 'node:events';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';

const [modulePath = 'docs/lab-app/hound.mjs', nativePath = 'build-host-web/simulator_worker'] = process.argv.slice(2);
const {default: createHound} = await import(pathToFileURL(resolve(modulePath)));
const hound = await createHound();
const independent = await createHound();
const native = spawn(resolve(nativePath), [], {stdio: ['pipe', 'pipe', 'inherit']});
await once(native, 'spawn');
const chunks = native.stdout[Symbol.asyncIterator]();
let buffered = Buffer.alloc(0);
let ms = 1000;
let frames = 0;
const deadline = setTimeout(() => {native.kill(); throw new Error('Simulator parity test timed out');}, 120000);
async function read(count) {
  while (count === null ? !buffered.includes(10) : buffered.length < count) {
    const {value, done} = await chunks.next();
    assert.ok(!done, 'Native worker closed before returning a frame');
    buffered = Buffer.concat([buffered, value]);
  }
  const length = count ?? buffered.indexOf(10) + 1;
  const value = buffered.subarray(0, length);
  buffered = buffered.subarray(length);
  return value;
}
async function step(action = 'frame', a = 0, b = 0, advance = 125) {
  ms += advance;
  native.stdin.write(`${action} ${a} ${b} ${ms}\n`);
  const state = JSON.parse(await read(null));
  const pixels = await read(307200);
  assert.equal(await hound.ccall('hound_step', 'number', ['string', 'number', 'number', 'number'],
    [action, a, b, ms], {async: true}), 1);
  const browserState = JSON.parse(hound.UTF8ToString(hound._hound_state()));
  assert.deepEqual(browserState, state, `${action}: native/browser state differs`);
  const start = hound._hound_pixels();
  assert.ok(pixels.equals(Buffer.from(hound.HEAPU8.subarray(start, start + 307200))), `${action}: native/browser pixels differ at frame ${frames}`);
  ++frames;
  return state;
}
try {
  assert.equal((await step()).width, 480);
  assert.equal((await step('tap', 330, 240)).paused, true);
  assert.equal((await step('inject', 0, 66)).events, 0);
  assert.equal((await step('tap', 330, 240)).paused, false);
  assert.ok((await step('inject', 0, 66)).preview_xp > 0);
  for (let i = 0; i < 16; ++i) await step('frame', 0, 0, 500);
  await step('pet_care', 0);
  assert.equal((await step()).fullness, 15);
  assert.ok((await step('inject', 1, 66)).fullness >= 35);
  await step('reset');
  await step('inject', 9, 66);
  assert.equal((await step('tap', 360, 201)).screen, 'Quiet alerts');
  assert.equal((await step('tap', 30, 180)).ignored, 1);
  assert.equal((await step('inject', 9, 66)).alert_active, false);
  assert.equal((await step('tag_watch_test', 9, 66)).tag_watch_warning, false);
  await step('reset');
  assert.equal((await step('tag_watch_test', 11, 66)).tag_watch_warning, true);
  await step('screen', 26);
  await step('screen', 27);
  await step('reset');
  assert.equal((await step('idle_test', 2)).display_mode, 1);
  assert.equal((await step('idle_test', 5)).display_mode, 2);
  assert.equal((await step('idle_test', 15)).display_mode, 3);
  assert.equal((await step('tap', 330, 240)).paused, false); // First tap only wakes.
  await step('reset');
  for (const category of Array.from({length: 19}, (_, i) => i + 4)) {
    await step('inject', category, 55);
    await step('frame', 0, 0, 4000);
  }
  for (const screen of [2,5,6,7,8,9,13,18,20,22,23,24,25,26,27,28]) await step('screen', screen);
  for (let breed = 0; breed < 6; ++breed) {
    await step('screen', 2);
    await step('tap', 20 + breed % 3 * 152, 80 + Math.floor(breed / 3) * 98);
    await step('screen', 5);
    await step('idle_test', 0);
    await step('frame', 0, 0, 1500);
    await step('tap', 200, 150);
    await step('screen', 24);
    await step('tap', 30, 76); // Theme row.
  }
  await step('screen', 8);
  await step('tap', 450, 255); // Next settings page.
  assert.equal((await step()).settings_page, 1);
  assert.equal((await step('tap', 30, 78)).rotation_locked, false);
  assert.equal((await step('rotate')).width, 320);
  for (const screen of [5,6,7,8,9,13,18,20,22,23,24,25,26,27,28]) await step('screen', screen);
  await step('inject', 0, 66);
  await step('frame', 0, 0, 2500);
  await step('idle_test', 0);
  await step('frame', 0, 0, 1500);
  await independent.ccall('hound_step', 'number', ['string','number','number','number'], ['frame',0,0,1000], {async:true});
  const fresh = JSON.parse(independent.UTF8ToString(independent._hound_state()));
  assert.equal(fresh.events, 0);
  assert.equal(fresh.ignored, 0);
  assert.equal(fresh.pet, 'CORGI');
  assert.equal(fresh.width, 480);
  await step('onboard');
  assert.equal((await step()).onboarded, false);
  assert.equal((await step('reset')).onboarded, true);
  console.log(`${frames} native/Wasm frames and states match; pause, meals, Ignore, Watch, idle, dogs, orientation and session isolation passed.`);
} finally {
  native.stdin.end();
  const [code, signal] = await once(native, 'close');
  clearTimeout(deadline);
  assert.equal(signal, null, 'Native worker was terminated unexpectedly');
  assert.equal(code, 0, 'Native worker failed during shutdown');
}
