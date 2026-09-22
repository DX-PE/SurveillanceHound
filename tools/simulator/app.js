// SPDX-License-Identifier: Apache-2.0
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', {alpha: false});
let pixels = context.createImageData(480, 320);
let requests = Promise.resolve();
let active = true;

async function display(response) {
  if (!response.ok) throw new Error(`Simulator returned ${response.status}`);
  const state = JSON.parse(response.headers.get('X-Sniffer-State'));
  const bytes = new DataView(await response.arrayBuffer());
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
  document.querySelector('#pet-name').textContent = state.name;
  document.querySelector('#pet-type').textContent = state.pet;
  document.querySelector('#mood').textContent = state.mood;
  document.querySelector('#fullness').textContent = state.fullness;
  document.querySelector('#events').textContent = state.events;
  document.querySelector('#state').textContent = `Device screen: ${state.screen} · ${state.paused ? "Sleeping" : "Sniffing"} · Saved XP: ${state.xp}`;
  document.querySelector('#connection').textContent = 'LOCAL SIMULATOR RUNNING';
}
function fail(error) {
  document.querySelector('#connection').textContent = 'CONNECTION LOST';
  document.querySelector('#state').textContent = error.message;
}
function request(action) {
  requests = requests.then(async () => {
    const response = action ? await fetch('/api/action', {
      method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(action)
    }) : await fetch('/api/frame');
    await display(response);
    if (action?.action === 'inject') {
      canvas.scrollIntoView({block: 'center', behavior: window.matchMedia('(prefers-reduced-motion: reduce)').matches ? 'auto' : 'smooth'});
    }
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
  button.addEventListener('click', () => request({action: button.dataset.action, a: Number(button.dataset.a || 0)}));
}
document.addEventListener('visibilitychange', () => {active = !document.hidden;});
async function tick() {
  if (active) await request();
  setTimeout(tick, 125);
}
tick();

document.querySelector('#sample-category').addEventListener('click', () => request({action: 'inject', a: Number(document.querySelector('#scent-category').value)}));
