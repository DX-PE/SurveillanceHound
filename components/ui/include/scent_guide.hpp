// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
namespace sniffer::ui {
struct ScentGuide {
    const char *about;
    const char *caution;
};
// Original summaries of the evidence already documented in docs/DETECTIONS.md.
constexpr ScentGuide scent_guide[] = {
    {"Plate-reader ecosystem clues from Wi-Fi names, vendor addresses or Bluetooth accessories.",
     "Shared ESP32/Liteon hardware is not proof of a Flock camera. Read the matched rule."},
    {"Axon ecosystem clues include body-camera names, vendor addresses and field accessories.",
     "A vendor or accessory clue does not identify the wearer or show that video is recording."},
    {"A Meta FD5F Bluetooth service clue, grouped here as a possible smart-glasses scent.",
     "Meta radio hardware can include other products. Reception does not prove recording."},
    {"Bluetooth serial-module names and services sometimes found in card skimmers.",
     "These are common hobby and industrial modules too. This is a weak clue, not a skimmer "
     "verdict."},
    {"Bluetooth services in the 3100-3500 family, sometimes associated with Raven equipment.",
     "Generic services are weak clues and may not be advertised. This does not establish a "
     "sensor."},
    {"Apple Find My offline-finding advertisements, also used by AirTags and other devices.",
     "This category is broader than AirTag. A broadcast does not establish unwanted tracking."},
    {"Bluetooth service clues associated with Tile finding devices.",
     "This does not identify an owner or intent. A changed radio address may look like a new "
     "device."},
    {"Samsung SmartTag-family Bluetooth service clues, including FD5A observations.",
     "A matching service is not an authenticated product identity or evidence of unwanted "
     "tracking."},
    {"Google Find Hub and related Bluetooth ecosystem clues.",
     "Fast Pair alone is broad and weak. Read which service actually matched before drawing "
     "conclusions."},
    {"Remote ID-shaped broadcasts and supported drone-related radio clues.",
     "Broadcast claims are unauthenticated. Locations, when received, are volatile local details."},
    {"Radio clues associated with automatic license-plate reader systems.",
     "Shared network hardware may match too. This cannot establish what a camera is capturing."},
    {"Wi-Fi vendor-address clues associated with camera manufacturers.",
     "Shared chips and IoT products create false positives. A match does not establish recording."},
    {"Doorbell and camera ecosystem clues associated with Ring.",
     "Vendor clues can cover other products. No account, owner or camera feed is accessed."},
    {"An Apple iBeacon-format Bluetooth advertisement.",
     "iBeacon is widely used for benign proximity services. Alerts are off by default."},
    {"Flipper-associated Bluetooth serial services or company-identifier clues.",
     "Names and services can be imitated. Possession of a Flipper does not indicate malicious "
     "use."},
    {"Structured Pwnagotchi-style Wi-Fi advertisements.",
     "Advertisements can be spoofed. This does not prove credential collection or an attack."},
    {"A Pineapple_ Wi-Fi network-name prefix associated with Wi-Fi Pineapple equipment.",
     "Names can be changed or copied. A match does not establish an active attack."},
    {"A burst of Wi-Fi deauthentication management frames within the detector window.",
     "Network faults and legitimate disconnects can cause bursts. This is a behavior clue."},
    {"Repeated same-name Wi-Fi observations with corroborating security differences.",
     "Legitimate deployments can differ too. This is a possible lookalike network, not proof."}};
static_assert(std::size(scent_guide) == category_count);
} // namespace sniffer::ui
