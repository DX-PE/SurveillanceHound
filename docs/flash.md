<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Install firmware

<div class="hound-hero" markdown>

<div class="hound-kicker">USB INSTALL · INITIAL ALPHA RELEASE</div>

## Bring your Hound home.

Install Surveillance Hound directly from your browser. Connect the supported board with a USB data cable, choose its serial port, and follow the installer.

**Currently supported:** Hosyond 4-inch / LCDWiki E32R40T — ESP32, 4 MB flash, ST7796 display and XPT2046 touch.

**Additional board support is coming soon.**

[Buy the tested board](https://amzn.to/4d4VI5R){ .md-button } (affiliate link)

</div>

## Before you connect

1. Use a desktop browser with Web Serial support, such as Chrome or Edge, on **HTTPS or localhost**.
2. Check the board model above. This image is not for ESP32-C3, ESP32-S3, or other ESP32 display boards. The installer checks the chip family; it cannot identify your display or board wiring.
3. Close serial monitors and other programs using the board. Use a USB **data** cable and keep it connected throughout installation.
4. If Hound is already running with a card, choose **Settings → Data → Eject SD** and wait for **SAFE TO REMOVE** before proceeding.

!!! warning "Full installation resets saved board data"

    This installer erases the board's flash and installs a factory image. Settings, ignored identities and game progress will reset. The SD card itself is not erased. SD ignore backups cannot restore identities after the board key is erased by this reset. To keep the current on-board state, use the [application-only update](#updating-an-existing-hound) instructions instead.

<div class="hound-installer" id="hound-installer">
  <p class="hound-kicker">HOSYOND 4-INCH / E32R40T · FULL INSTALL</p>
  <p><strong id="hound-firmware-version">Checking release…</strong></p>
  <label class="hound-install-confirm">
    <input type="checkbox" id="hound-install-confirm" disabled>
    <span>I have the supported board and understand that this installation resets its saved data.</span>
  </label>
  <esp-web-install-button id="hound-install" hidden>
    <button type="button" slot="activate" class="md-button md-button--primary" id="hound-connect" disabled>Connect and install</button>
    <span slot="unsupported">Use a desktop browser with Web Serial support, such as Chrome or Edge.</span>
    <span slot="not-allowed">Open this page over HTTPS or localhost to install.</span>
  </esp-web-install-button>
  <p id="hound-install-status" role="status" aria-live="polite">Checking browser support and release files…</p>
  <noscript><p>Enable JavaScript to use the browser installer, or download the release and use the manual installation instructions below.</p></noscript>
</div>

<script type="module" src="javascripts/flasher.js"></script>

[Download release files](https://github.com/DX-PE/SurveillanceHound/releases/tag/v0.1.0-alpha.1){ .md-button }
[Try the browser lab first](lab.md){ .md-button }

The installer uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/) 10.4.0, loaded from jsDelivr. Firmware is served from this documentation site and checked against the release SHA-256 during the site build. USB access begins only when you press **Connect and install** and select a port.

## After installation

Wait for installation to finish, then reset or power-cycle the board if needed. Complete first-boot touch calibration, region/privacy settings and dog setup. The tested Hosyond uses **Panel inversion: Off**. See [Getting started](getting-started.md) for controls, SD setup and detection limits.

This is the initial **0.1.0-alpha.1 prerelease**. The device About screen reports `0.1.0`; the release package identifies the alpha. Controlled RF accuracy, power/durability testing and formal release acceptance remain open. Read [current status](STATUS.md) before relying on detections.

## Updating an existing Hound

The browser installer always performs a full installation. For an existing Hound with the same partition layout, download the **esp32-e32r40t.zip** from the release, verify `SHA256SUMS`, and follow its `INSTALL.md` application-only update at offset `0x10000`. This avoids intentionally overwriting NVS. Keep a backup of important data and safely eject SD first. The standalone **app.bin** is the same application image; it is not a factory image and must not be written at offset zero.

For a source build or a different development profile, use [Build firmware](BUILDING.md).

## Connection help

- **No port appears:** try another USB data cable or port; install the driver appropriate to your board's USB serial chip if the operating system does not recognize it.
- **Port busy or permission denied:** close other browser installers and serial monitors. On Linux, ensure your user has access to the serial device.
- **Cannot connect:** follow the board's bootloader procedure, then retry. Do not disconnect during an erase or write.
- **Unsupported chip:** check the exact board model. Do not flash this image onto a different board just because it also uses an ESP32.
- **Installer fails to load:** allow the pinned ESP Web Tools module from jsDelivr or use the release's manual instructions. Release downloads remain available without the browser installer.

## Firmware verification

The [manifest](firmware/manifest.json) selects only the ESP32 factory image at offset zero. [Release metadata](firmware/release.json) records the source commit, image size and SHA-256. These files and the image are hosted together, so installation does not depend on cross-origin downloads from GitHub.
