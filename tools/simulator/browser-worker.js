// SPDX-License-Identifier: Apache-2.0
import createHound from './hound.mjs';

const ready = createHound();
let pending = Promise.resolve();
// Asyncify calls must finish before another call enters the same Wasm instance.
self.onmessage = ({data: {id, action, ms}}) => {
  pending = pending.then(async () => {
    try {
      const hound = await ready;
      const ok = await hound.ccall('hound_step', 'number', ['string', 'number', 'number', 'number'],
        [action?.action ?? 'frame', action?.a ?? 0, action?.b ?? 0, ms], {async: true});
      if (!ok) throw new Error('Invalid simulator command');
      const state = JSON.parse(hound.UTF8ToString(hound._hound_state()));
      const start = hound._hound_pixels();
      const frame = hound.HEAPU8.slice(start, start + 480 * 320 * 2).buffer;
      self.postMessage({id, state, frame}, [frame]);
    } catch (error) {
      self.postMessage({id, error: error.message || String(error)});
    }
  });
};
