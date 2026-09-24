// SPDX-License-Identifier: Apache-2.0
// Only this page loads the pinned installer. No port is requested on page load.
const panel = document.querySelector('#hound-installer');
const confirm = document.querySelector('#hound-install-confirm');
const installer = document.querySelector('#hound-install');
const button = document.querySelector('#hound-connect');
const status = document.querySelector('#hound-install-status');
const version = document.querySelector('#hound-firmware-version');
const firmwareRoot = new URL('../firmware/', import.meta.url);
let ready = false;

function update() {
  button.disabled = !ready || !confirm.checked;
  if (ready) {
    status.textContent = confirm.checked
      ? 'Ready. Choose your board’s USB serial port to continue.'
      : 'Confirm the board and data reset above to enable installation.';
  }
}

// Gate the custom element as well as the button; upstream listens on its slot.
installer.addEventListener('click', event => {
  if (!ready || !confirm.checked) {
    event.preventDefault();
    event.stopImmediatePropagation();
  }
}, true);
confirm.addEventListener('change', update);

async function initialize() {
  if (!window.isSecureContext) {
    version.textContent = 'Browser installation needs HTTPS';
    status.textContent = 'Open this page over HTTPS or localhost. Manual release downloads are available below.';
    return;
  }
  if (!('serial' in navigator)) {
    version.textContent = 'Web Serial is unavailable in this browser';
    status.textContent = 'Open this page in a desktop browser with Web Serial, such as Chrome or Edge, or use the manual release downloads below.';
    return;
  }

  const [releaseResponse, manifestResponse] = await Promise.all([
    fetch(new URL('release.json', firmwareRoot), {signal: AbortSignal.timeout(15000)}),
    fetch(new URL('manifest.json', firmwareRoot), {signal: AbortSignal.timeout(15000)})
  ]);
  if (!releaseResponse.ok || !manifestResponse.ok) throw new Error('Release files unavailable');
  const release = await releaseResponse.json();
  const manifest = await manifestResponse.json();
  const build = manifest.builds?.[0];
  if (release.chip !== 'ESP32' || release.offset !== 0 ||
      manifest.version !== release.version || manifest.builds.length !== 1 ||
      build?.chipFamily !== 'ESP32' || build.parts.length !== 1 ||
      build.parts[0].path !== release.filename || build.parts[0].offset !== 0 ||
      manifest.new_install_prompt_erase !== false || manifest.new_install_improv_wait_time !== 0) {
    throw new Error('Release metadata does not match the installer');
  }
  version.textContent = `Surveillance Hound ${release.version}`;
  installer.manifest = new URL('manifest.json', firmwareRoot).href;

  await new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.type = 'module';
    script.src = 'https://cdn.jsdelivr.net/npm/esp-web-tools@10.4.0/dist/web/install-button.js';
    script.integrity = 'sha384-9XfyvAabgkISlB/Xb3OpshGKpZx0TOt5Mmu78SeiNU3so/p4o2s9m66vy6Utdnom';
    script.crossOrigin = 'anonymous';
    const timer = setTimeout(() => reject(new Error('Installer loading timed out')), 30000);
    script.onload = () => {
      clearTimeout(timer);
      if (customElements.get('esp-web-install-button')) resolve();
      else reject(new Error('Installer did not initialize'));
    };
    script.onerror = () => {
      clearTimeout(timer);
      reject(new Error('Installer module unavailable'));
    };
    panel.append(script);
  });
  installer.hidden = false;
  confirm.disabled = false;
  confirm.checked = false;
  ready = true;
  update();
}

initialize().catch(() => {
  ready = false;
  confirm.disabled = true;
  installer.hidden = true;
  button.disabled = true;
  status.textContent = 'The browser installer could not load. Reload to retry, or use the release downloads and manual instructions below.';
});
