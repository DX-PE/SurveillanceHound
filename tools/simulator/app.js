// SPDX-License-Identifier: Apache-2.0
import {createTransport} from './transport.js';
let transport;
let stopped = false;
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', {alpha: false});
let pixels = context.createImageData(480, 320);
let requests = Promise.resolve();
let active = true;
let following = false;
let lastSignal = 0;

function display({state, frame}) {
  const bytes = new DataView(frame);
  if (![[480, 320], [320, 480]].some(([w, h]) => w === state.width && h === state.height)) throw new Error('Invalid display size');
  if (canvas.width !== state.width || canvas.height !== state.height) {
    canvas.width = state.width;
    canvas.height = state.height;
    pixels = context.createImageData(state.width, state.height);
  }
  canvas.setAttribute('aria-label', `Interactive ${state.width} by ${state.height} device touchscreen`);
  document.querySelector('.device').classList.toggle('portrait', state.height > state.width);
  document.querySelector('#rotate').disabled = state.rotation_locked;
  document.querySelector('#orientation').textContent = `${state.width > state.height ? 'LANDSCAPE' : 'PORTRAIT'} / ${state.rotation_locked ? 'LOCKED' : 'UNLOCKED'}`;
  if (bytes.byteLength !== 320 * 480 * 2) throw new Error('Incomplete display frame');
  for (let i = 0; i < 320 * 480; i++) {
    const value = bytes.getUint16(i * 2, true);
    pixels.data[i * 4] = ((value >> 11) & 31) * 255 / 31;
    pixels.data[i * 4 + 1] = ((value >> 5) & 63) * 255 / 63;
    pixels.data[i * 4 + 2] = (value & 31) * 255 / 31;
    pixels.data[i * 4 + 3] = 255;
  }
  context.putImageData(pixels, 0, 0);
  document.querySelector('#watch-status').textContent = state.tag_watch_warning ? 'Possible following: repeated tag presence. Tap REVIEW TAG to acknowledge, ignore or snooze.' : state.tag_watch_armed ? 'Travel Watch armed. No active warning.' : 'Watch is off.';
  document.querySelector('#preview-tag-watch').disabled = state.paused;
  following = state.following;
  document.querySelector('#send-signal').disabled = !following || state.paused;
  document.querySelector('#pet-name').textContent = state.name;
  document.querySelector('#pet-type').textContent = state.pet;
  document.querySelector('#mood').textContent = state.mood;
  document.querySelector('#fullness').textContent = state.fullness;
  document.querySelector('#events').textContent = state.events;
  document.querySelector('#state').textContent = `Device screen: ${state.screen} · Display: ${["awake", "dimmed", "bouncing hound", "off"][state.display_mode]} · ${state.paused ? "Sleeping" : "Sniffing"} · Demo XP: ${state.preview_xp} · Scents: ${state.discoveries}/19${state.snoozed ? " · Alerts snoozed" : ""}`;
  document.querySelector('#connection').textContent = transport.label;
}
function fail(error) {
  stopped = true;
  document.querySelector('#connection').textContent = 'LAB UNAVAILABLE';
  document.querySelector('#state').textContent = error.message;
}
function request(action) {
  requests = requests.then(async () => {
    if (stopped) return;
    display(await transport.request(action));
  }).catch(fail);
  return requests;
}
canvas.addEventListener('pointerdown', event => {
  event.preventDefault();
  const bounds = canvas.getBoundingClientRect();
  request({action: 'tap', a: Math.min(canvas.width - 1, Math.max(0, Math.floor((event.clientX - bounds.left) * canvas.width / bounds.width))),
           b: Math.min(canvas.height - 1, Math.max(0, Math.floor((event.clientY - bounds.top) * canvas.height / bounds.height)))});
});
canvas.addEventListener('keydown', event => {
  if (event.key === 'b' || event.key === 'B') request({action: 'boot'});
});
document.querySelector('#boot').addEventListener('click', () => request({action: 'boot'}));
for (const button of document.querySelectorAll('[data-action]')) {
  button.addEventListener('click', () => request({action: button.dataset.action, a: Number(button.dataset.a || 0), b: Number(document.querySelector('#signal-level').value)}));
}
document.addEventListener('visibilitychange', () => {active = !document.hidden;});
async function tick() {
  if (stopped) return;
  if (active) {
    if (following && document.querySelector('#repeat-signal').checked && Date.now() - lastSignal >= 1000) {
      lastSignal = Date.now();
      await request({action: 'signal', b: Number(document.querySelector('#signal-level').value)});
    } else await request();
  }
  setTimeout(tick, 125);
}
try {
  transport = createTransport(document.body.dataset.runtime);
  tick();
} catch (error) {fail(error);}
window.addEventListener('pagehide', () => {stopped = true; transport?.close();});
window.addEventListener('pageshow', event => {if (event.persisted) location.reload();});

document.querySelector('#sample-category').addEventListener('click', () => request({action: 'inject', a: Number(document.querySelector('#scent-category').value), b: Number(document.querySelector('#signal-level').value)}));

const signalLevel = document.querySelector('#signal-level');
signalLevel.addEventListener('input', () => {document.querySelector('#signal-reading').textContent = `-${signalLevel.value} dBm`;});
document.querySelector('#send-signal').addEventListener('click', () => request({action: 'signal', b: Number(signalLevel.value)}));

// Synthetic-only time jump; live firmware still requires actual elapsed observation time.
document.querySelector('#preview-tag-watch').addEventListener('click', () => request({action: 'tag_watch_test', a: Number(document.querySelector('#watch-category').value), b: Number(signalLevel.value)}));
