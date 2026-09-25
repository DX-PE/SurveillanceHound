<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Documentation development

The site uses **Zensical 0.0.65** with a pinned Python toolchain in `requirements-docs.txt`. Markdown remains the source format. The Midnight palette comes from the firmware; the six-dog artwork is generated from the original sprite geometry.

## Install and preview

Use Python 3.12 and Node.js 22 (the CI versions), or compatible newer versions:

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
python tools/build_web.py --docker
python tools/build_flasher.py
npm ci --prefix tools/flasher --ignore-scripts
npm run build --prefix tools/flasher
zensical serve
```

Docker must be available for `--docker`; it downloads the pinned Emscripten 6.0.10 compiler image on first use. Alternatively activate that exact Emscripten SDK and run `python tools/build_web.py` without Docker.

Open <http://127.0.0.1:8000/lab/> for the embedded lab or <http://127.0.0.1:8000/lab-app/index.html> for its full-window view. The native local emulator continues to use port 8765. The preview binds to loopback and rebuilds when documentation changes.

## Build and check

```sh
python3 tools/build_web.py --docker
python3 tools/build_flasher.py
npm ci --prefix tools/flasher --ignore-scripts
npm test --prefix tools/flasher
npm run build --prefix tools/flasher
python3 tools/docs_assets.py --check
python3 tools/check_docs.py
zensical build --clean --strict
python3 tools/check_docs.py --site site
python3 tools/build_flasher.py --check --output site/firmware
```

`site/` is the static output. Preview an existing build with `python3 -m http.server 8000 --bind 127.0.0.1 --directory site`. Generated output, caches and the local environment are ignored by Git.

Zensical validates Markdown links and anchors in strict mode. The project check verifies navigation coverage and single-source imports, then checks generated local page links, assets and anchors after the build. It does not claim external research URLs are reachable or that detector facts are accurate.

## Browser simulator

`tools/build_web.py` compiles the shared synthetic simulator and firmware UI to `docs/lab-app/`. This generated directory is ignored by Git and copied into the static site by Zensical. Rebuild it after C++, controls or lab-style changes; Zensical's own live reload rebuilds documentation only. Build the lab before `zensical build --strict` so all links resolve.

The browser creates one Web Worker and one WebAssembly module per lab, serializes actions around asynchronous Web Crypto, and transfers RGB565 frames back to the existing canvas. No Python API, shared server process, cross-origin isolation headers, radio permissions or persistent browser storage are needed. Assets use relative URLs so both a Pages project path and a custom domain work. This is a UI/game simulation, not a radio or ESP32 CPU emulator.

Compare identical commands and every rendered pixel against the native simulator with Node.js 20 or newer:

```sh
cmake -S . -B build-host-web -G Ninja -DSNIFFER_HOST=ON -DSNIFFER_SANITIZE=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host-web --target simulator_worker
node tools/test_web.mjs
```

Tests cover meals, pause/resume, Ignore, Watch, idle dim/saver/off, all sample categories, six dogs, both orientations, reset and independent sessions. Runtime license notices are included in `lab-app/LICENSES.txt`.

## Browser firmware installer

`tools/build_flasher.py` downloads the factory image named in `tools/firmware-release.json`, verifies its exact size and SHA-256, then stages `docs/firmware/` with an ESP Web Tools manifest and release metadata. These generated assets are ignored by Git. An existing image is verified before reuse; `--firmware-file /path/to/factory.bin` supports offline staging of a previously downloaded release. `--check --output site/firmware` verifies the final published bytes and manifest without downloading anything.

Publish and verify the release first, then update the pinned metadata for a new version. Do not use a moving `latest` URL. Keep `docs/flash.md` and README release links in sync. The manifest accepts only the ESP32 family and selects a full factory image at offset zero. It disables Improv Wi-Fi setup and optional erase prompts; the page explicitly explains the reset and requires board/data acknowledgement before enabling USB connection. Chip matching cannot identify the display board model.

The page loads a self-hosted ESP Web Tools 10.4.0 bundle built with esptool-js 0.7.0. `tools/flasher/package-lock.json` pins dependencies; `npm ci --ignore-scripts` installs them and `npm run build` stages ignored `docs/flasher-vendor/` assets with full dependency license notices. Material Web is held at 2.2.0 for the upstream component imports. Only the flasher page loads this dependency.

The integration adds SHA-256 and size verification of the downloaded bytes before erasing, then requests the device MD5 over the complete written image before the upstream success state. MD5 is the bootloader transfer-integrity check, not the release authenticity mechanism. A mismatch or serial error follows the installation error path. A source hash guard makes the build fail if the patched upstream flash routine changes; review its error and completion paths before updating the pin. Tests exercise that actual upstream state machine with simulated serial I/O, including a write that returns successfully but has incomplete flash contents.

Firmware itself is served from the documentation origin, avoiding GitHub release-download CORS requirements. If Web Serial, a secure context or the installer is unavailable, manual release downloads remain visible. This feature does not change the passive firmware or request a USB port on page load.

Run `python -m unittest discover -s tests/host -p test_flasher.py`, `node --test tools/test_flasher.mjs`, `npm test --prefix tools/flasher`, and the final asset check when changing installation metadata or verification code. In a browser, check acknowledgement gating, manual fallback, layout and external links without opening or flashing a board unless hardware testing is intended.

## Where to edit

| Content | Source |
| --- | --- |
| User guides and technical references | The corresponding Markdown file in `docs/` |
| Site title, navigation and features | `zensical.toml` |
| Theme and responsive tables | `docs/stylesheets/hound.css` |
| Browser firmware page and controller | `docs/flash.md`, `docs/javascripts/flasher.js` |
| Verified installer bundle and tests | `tools/flasher/` |
| Released firmware pin and staging | `tools/firmware-release.json`, `tools/build_flasher.py` |
| External links in new tabs | `docs/javascripts/external-links.js` |
| Accessible missing-page template | `docs-overrides/404.html` |
| Header dog / six-dog illustration | `tools/docs_assets.py`, reusing `tools/pack_assets.py` and `assets/pets/characters.json` |
| Dependency ledger | Root `THIRD_PARTY_LICENSES.md`, included by `docs/licenses.md` |
| Asset credit, device worksheet and fixture policy | Their original files under `assets/` and `tests/`, included in site wrapper pages |
| Contributor entry point | Root `CONTRIBUTING.md` links to the full site guide |

The outside-document imports use `pymdownx.snippets` with missing-file checks. They render their original source without maintaining a second copy. Relative links in an included source resolve from its wrapper page; use documentation-relative links or plain checkout paths accordingly. Do not include private backups or arbitrary local directories.

Add every new page to `project.nav`. Use relative `.md` links inside the site so Zensical can generate the correct page URLs. Preserve existing anchors when possible. Place chronological measurements in Engineering records; keep current instructions and release status in the main guides.

Regenerate artwork after changing the underlying dog geometry:

```sh
python3 tools/docs_assets.py
```

## GitHub workflow


`.github/workflows/docs.yml` builds the browser lab and verified installer, stages checksum-verified released firmware for the flasher, checks native/browser parity, builds the documentation strictly, validates the resulting site, and uploads one `hound-docs` artifact on pushes, pull requests and manual dispatch.

Pushes to the default branch of [DX-PE/SurveillanceHound](https://github.com/DX-PE/SurveillanceHound) publish the checked artifact to GitHub Pages. Pull requests only build and test. The deployment job uses the `github-pages` environment and narrowly scoped Pages/OIDC permissions.

The canonical site URL is **https://surveillancehound.dx.pe/**, configured as `project.site_url` in `zensical.toml`. Hosting setup:

1. Select **Settings → Pages → Source: GitHub Actions**.
2. Set the Pages **Custom domain** to **surveillancehound.dx.pe**.
3. In the `dx.pe` DNS zone, point the `surveillancehound` CNAME to **dx-pe.github.io** (without the repository name).
4. Enable **Enforce HTTPS** once GitHub provisions the domain certificate. The browser lab needs HTTPS, or localhost, for Web Crypto.
5. Push the default branch or manually run the documentation workflow from that branch.

For an Actions deployment, GitHub stores the custom domain in the repository's Pages settings; a source `CNAME` file is not required. See [GitHub's custom-domain instructions](https://docs.github.com/en/pages/configuring-a-custom-domain-for-your-github-pages-site/managing-a-custom-domain-for-your-github-pages-site#configuring-a-subdomain). The same artifact can be served by any static host; it contains both docs and the working lab. A local build alone does not verify DNS, certificates or remote deployment.

## Configuration reference

The navigation, dark/light toggle and code-copy setup were informed by [RelayFabric's configuration](https://github.com/RelayFabric/RelayFabric/blob/main/zensical.toml) and [documentation workflow](https://github.com/RelayFabric/RelayFabric/blob/main/.github/workflows/docs.yml). Hound uses its own colors, artwork and content, with a pinned toolchain and pull-request build checks.

See the official [Zensical configuration](https://zensical.org/docs/setup/basics/) and [validation documentation](https://zensical.org/docs/setup/validation/) when updating the site toolchain.
