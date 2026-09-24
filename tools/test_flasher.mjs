// SPDX-License-Identifier: Apache-2.0
// Controller tests only: no browser, serial device or third-party code is opened.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {setImmediate} from 'node:timers/promises';
import test from 'node:test';
import vm from 'node:vm';

const source = readFileSync(new URL('../docs/javascripts/flasher.js', import.meta.url), 'utf8')
  .replace('import.meta.url', JSON.stringify('https://example.invalid/project/javascripts/flasher.js'));

async function run(options = {}) {
  const nodes = new Map();
  for (const id of ['hound-installer', 'hound-install-confirm', 'hound-install', 'hound-connect', 'hound-install-status', 'hound-firmware-version']) {
    nodes.set(`#${id}`, {disabled: true, checked: false, hidden: true, listeners: {},
      addEventListener(name, fn) { this.listeners[name] = fn; }});
  }
  const requested = [];
  let serialRequests = 0;
  let scripts = 0;
  const release = {version: '0.1.0-alpha.1', chip: 'ESP32', offset: 0, filename: 'factory.bin'};
  const manifest = {version: release.version, new_install_prompt_erase: false, new_install_improv_wait_time: 0,
    builds: [{chipFamily: options.wrongChip ? 'ESP32-C3' : 'ESP32', parts: [{path: 'factory.bin', offset: 0}]}]};
  nodes.get('#hound-installer').append = script => {
    scripts++;
    assert.equal(script.src, 'https://cdn.jsdelivr.net/npm/esp-web-tools@10.4.0/dist/web/install-button.js');
    assert.match(script.integrity, /^sha384-/);
    queueMicrotask(() => options.loadFails ? script.onerror() : script.onload());
  };
  vm.runInNewContext(source, {
    URL, AbortSignal, setTimeout, clearTimeout,
    window: {isSecureContext: options.secure !== false},
    navigator: options.serial === false ? {} : {serial: {requestPort() { serialRequests++; }}},
    document: {querySelector: id => nodes.get(id), createElement: () => ({})},
    customElements: {get: () => class Installer {}},
    fetch: async url => {
      requested.push(url.href);
      return {ok: !options.missingFiles, json: async () => url.pathname.endsWith('release.json') ? release : manifest};
    }
  });
  // Flush the controller's promise chain, without a timing-sensitive sleep.
  await setImmediate();
  await setImmediate();
  return {nodes, requested, scripts, serialRequests};
}

test('unsupported and insecure browsers retain fallback without downloads or USB access', async () => {
  for (const options of [{secure: false}, {serial: false}]) {
    const result = await run(options);
    assert.equal(result.scripts, 0);
    assert.equal(result.serialRequests, 0);
    assert.equal(result.requested.length, 0);
    assert.equal(result.nodes.get('#hound-install-confirm').disabled, true);
    assert.match(result.nodes.get('#hound-install-status').textContent, /downloads/);
  }
});

test('missing metadata, wrong chip, and failed module load leave installation disabled', async () => {
  for (const options of [{missingFiles: true}, {wrongChip: true}, {loadFails: true}]) {
    const result = await run(options);
    assert.equal(result.serialRequests, 0);
    assert.equal(result.nodes.get('#hound-connect').disabled, true);
    assert.equal(result.nodes.get('#hound-install').hidden, true);
    assert.match(result.nodes.get('#hound-install-status').textContent, /could not load/);
  }
});

test('acknowledgement gates clicks, can be withdrawn, and works below a site prefix', async () => {
  const {nodes, requested, serialRequests} = await run();
  const confirm = nodes.get('#hound-install-confirm');
  const button = nodes.get('#hound-connect');
  const installer = nodes.get('#hound-install');
  assert.equal(serialRequests, 0);
  assert.deepEqual(requested, ['https://example.invalid/project/firmware/release.json', 'https://example.invalid/project/firmware/manifest.json']);
  assert.equal(installer.manifest, 'https://example.invalid/project/firmware/manifest.json');
  assert.equal(confirm.disabled, false);
  assert.equal(button.disabled, true);
  let stopped = false;
  installer.listeners.click({preventDefault() {}, stopImmediatePropagation() { stopped = true; }});
  assert.equal(stopped, true);
  confirm.checked = true;
  confirm.listeners.change();
  assert.equal(button.disabled, false);
  confirm.checked = false;
  confirm.listeners.change();
  assert.equal(button.disabled, true);
});
