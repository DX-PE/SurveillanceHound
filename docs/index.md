<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound

<div class="hound-hero" markdown>

<div class="hound-kicker">PASSIVE OBSERVATION · A LITTLE PERSONALITY</div>

## Good dog. Sharp nose.

An offline ESP32 companion that notices nearby Wi-Fi and Bluetooth clues, keeps local logs, and turns each discovery into a snack for your hound.

[Get started](getting-started.md){ .md-button .md-button--primary }
[Try the Hound](lab.md){ .md-button }
[Install firmware](flash.md){ .md-button }

![Six original pixel hounds: Corgi, Beagle, Shiba, Husky, Labrador and Dalmatian](images/hound-pack.svg){ .hound-pack }

</div>

## Pick your trail

<div class="grid cards" markdown>

- **Use your Hound**

    Controls, outfits, quiet alerts and the bouncing dog screen saver.

    [Open the user guide](controls.md)

- **Understand a scent**

    67 rules across 19 categories, with the evidence and limits explained.

    [Read detection coverage](DETECTIONS.md)

- **Follow and watch**

    Explore signal reception or watch for a tag that remains nearby.

    [Learn about Travel Watch](tag-watch.md)

- **Build and contribute**

    Build the firmware, run the host tests, or improve the documentation.

    [Start contributing](CONTRIBUTING.md)

</div>

## What it does

Six dogs, local discoveries, earned outfits and a passive receiver for selected 2.4 GHz Wi-Fi management frames and legacy BLE advertisements. The application does not associate, probe, advertise, inject frames or upload observations. [See all features](features.md).

!!! info "Initial alpha release · 0.1.0-alpha.1"

    Initial hardware checks and host regression tests are recorded. Controlled RF accuracy, card/power interruption checks and formal release acceptance remain open. See [current status](STATUS.md) and [release gates](RELEASE.md).

A match describes radio evidence, not intent or proof of surveillance. Some equipment emits nothing Hound can recognize; **no matching signals is not an all-clear**.
