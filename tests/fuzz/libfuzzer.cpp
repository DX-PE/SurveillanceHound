// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include "sniffer/json.hpp"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    sniffer::Observation o{};
    sniffer::SamsungTag samsung{};
    sniffer::parse_samsung_tag({data, size}, samsung);
    sniffer::RemoteId rid{};
    sniffer::parse_wifi({data, size}, o);
    sniffer::parse_ble({data, size}, o);
    sniffer::parse_remote_id({data, size}, rid);
    char name[33];
    sniffer::parse_pwnagotchi({data, size}, name);
    std::string_view text(reinterpret_cast<const char *>(data), size);
    sniffer::Json json;
    json.parse(text);
    sniffer::valid_log_record(text);
    std::array<char, 2048> output{};
    std::array<uint8_t, 32> key{};
    sniffer::export_log_record(text, key, output);
    return 0;
}
