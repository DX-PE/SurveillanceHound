// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include "sniffer/json.hpp"
#include <algorithm>
#include <cstring>

namespace sniffer {
namespace {
uint16_t le16(const uint8_t *p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
int32_t le32s(const uint8_t *p) {
    uint32_t v =
        uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    return static_cast<int32_t>(v);
}
bool rsn_valid(std::span<const uint8_t> v, uint16_t &caps) {
    if (v.size() < 8 || le16(v.data()) != 1)
        return false;
    size_t pos = 6;
    for (int group = 0; group < 2; ++group) {
        if (pos + 2 > v.size())
            return false;
        size_t n = le16(v.data() + pos);
        pos += 2;
        if (!n || n > (v.size() - pos) / 4)
            return false;
        pos += n * 4;
    }
    if (pos == v.size()) {
        caps = 0;
        return true;
    }
    if (v.size() - pos < 2)
        return false;
    caps = le16(v.data() + pos);
    return true;
}
} // namespace
void sanitize(std::span<const uint8_t> bytes, std::span<char> dst) {
    if (dst.empty())
        return;
    size_t n = std::min(bytes.size(), dst.size() - 1);
    for (size_t i = 0; i < n; ++i)
        dst[i] = bytes[i] >= 32 && bytes[i] <= 126 ? char(bytes[i]) : '?';
    dst[n] = 0;
}
bool parse_wifi(std::span<const uint8_t> frame, Observation &output) {
    if (frame.size() < 24 || (frame[0] & 0x0f) != 0 || (frame[1] & 0x03) != 0)
        return false;
    Observation o{};
    o.ms = output.ms;
    o.channel = output.channel;
    o.rssi = output.rssi;
    o.radio = Radio::Wifi;
    o.subtype = frame[0] >> 4;
    std::copy_n(frame.data() + 10, 6, o.address.begin());
    if (o.subtype == 10 || o.subtype == 12) {
        // Encrypted management bodies cannot be interpreted as a clear reason code.
        if (frame.size() < 26 || (frame[1] & 0x40))
            return false;
        output = o;
        return true;
    }
    if ((o.subtype != 8 && o.subtype != 5 && o.subtype != 4) ||
        frame.size() < (o.subtype == 4 ? 24U : 36U))
        return false;
    o.protected_network = o.subtype != 4 && (le16(frame.data() + 34) & 0x10) != 0;
    std::array<uint8_t, 472> pwn{};
    size_t pwn_size = 0;
    bool ssid_seen = false;
    for (size_t i = o.subtype == 4 ? 24 : 36; i < frame.size();) {
        if (frame.size() - i < 2)
            return false;
        uint8_t type = frame[i++], n = frame[i++];
        if (n > frame.size() - i)
            return false;
        auto v = frame.subspan(i, n);
        i += n;
        if (type == 0) {
            if (ssid_seen || n > 32)
                return false;
            ssid_seen = true;
            o.ssid_len = n;
            std::copy(v.begin(), v.end(), o.ssid.begin());
            sanitize(v, o.name);
        } else if (type == 48) {
            if (o.rsn || !rsn_valid(v, o.rsn_caps))
                return false;
            o.rsn = true;
            o.protected_network = true;
        } else if (type == 222) {
            if (pwn_size + n > pwn.size())
                return false;
            std::copy(v.begin(), v.end(), pwn.begin() + pwn_size);
            pwn_size += n;
        } else if (type == 221 && n >= 3 && o.vendor_len == 0) {
            o.vendor_fingerprint = 2166136261U;
            for (size_t j = 0; j < std::min<size_t>(4, v.size()); ++j)
                o.vendor_fingerprint = (o.vendor_fingerprint ^ v[j]) * 16777619U;
            o.vendor_len = static_cast<uint8_t>(std::min<size_t>(n, o.vendor.size()));
            std::copy_n(v.begin(), o.vendor_len, o.vendor.begin());
        }
    }
    if (pwn_size && o.subtype == 8) {
        constexpr Address mesh{0xde, 0xad, 0xbe, 0xef, 0xde, 0xad};
        o.pwnagotchi =
            o.address == mesh && parse_pwnagotchi(std::span(pwn).first(pwn_size), o.name);
        if (o.pwnagotchi)
            std::copy_n(frame.data() + 16, 6, o.address.begin());
    }
    if (!ssid_seen && !o.pwnagotchi)
        return false;
    output = o;
    return true;
}
bool parse_samsung_tag(std::span<const uint8_t> service, SamsungTag &output) {
    // FD5A + the documented 20-byte v1/type-0 payload. See docs/SAMSUNG_TAGS.md.
    // Do not interpret an unfamiliar/partial packet as an owner connection.
    if (service.size() != 22 || service[0] != 0x5a || service[1] != 0xfd ||
        (service[2] & 0xf8) != 0x10)
        return false;
    SamsungTag tag{};
    std::copy_n(service.begin() + 6, tag.id.size(), tag.id.begin());
    if (std::all_of(tag.id.begin(), tag.id.end(), [](auto b) { return b == 0; }) ||
        std::all_of(tag.id.begin(), tag.id.end(), [](auto b) { return b == 0xff; }))
        return false;
    switch (service[2] & 7) {
    case 1:
        tag.state = SamsungState::RecentlySeparated;
        break;
    case 2:
        tag.state = SamsungState::Offline;
        break;
    case 3:
        tag.state = SamsungState::LongOffline;
        break;
    case 5:
        tag.state = SamsungState::Connected;
        break;
    default:
        tag.state = SamsungState::Unknown;
        break;
    }
    output = tag;
    return true;
}
bool parse_ble(std::span<const uint8_t> data, Observation &output) {
    if (data.size() > 31)
        return false; // Legacy advertising only, no scan responses.
    Observation o{};
    o.ms = output.ms;
    o.rssi = output.rssi;
    o.address = output.address;
    o.address_type = output.address_type;
    o.radio = Radio::Ble;
    bool full_name = false;
    for (size_t i = 0; i < data.size();) {
        uint8_t n = data[i++];
        if (n == 0) {
            if (!std::all_of(data.begin() + i, data.end(), [](auto b) { return b == 0; }))
                return false;
            break;
        }
        if (n > data.size() - i)
            return false;
        uint8_t type = data[i];
        auto v = data.subspan(i + 1, n - 1);
        i += n;
        if (type == 2 || type == 3) {
            if (v.size() % 2)
                return false;
            for (size_t j = 0; j < v.size(); j += 2)
                if (o.uuid_count < o.uuids.size())
                    o.uuids[o.uuid_count++] = le16(v.data() + j);
        } else if (type == 6 || type == 7 || type == 0x21) {
            if (type == 0x21 ? v.size() < 16 : v.size() % 16 != 0)
                return false;
            for (size_t j = 0; j + 16 <= v.size() && o.uuid128_count < o.uuids128.size(); j += 16) {
                std::reverse_copy(v.begin() + j, v.begin() + j + 16,
                                  o.uuids128[o.uuid128_count++].begin());
                if (type == 0x21)
                    break;
            }
        } else if (type == 8 || type == 9) {
            if (type == 9 || !full_name)
                sanitize(v, o.name);
            full_name |= type == 9;
        } else if (type == 0xff) {
            if (v.size() < 2 || o.manufacturer_len)
                return false;
            o.manufacturer_len = static_cast<uint8_t>(v.size());
            std::copy(v.begin(), v.end(), o.manufacturer.begin());
        } else if (type == 0x16) {
            if (v.size() < 2 || o.service_len)
                return false;
            o.service_len = static_cast<uint8_t>(v.size());
            std::copy(v.begin(), v.end(), o.service.begin());
            if (o.uuid_count < o.uuids.size())
                o.uuids[o.uuid_count++] = le16(v.data());
        }
    }
    parse_samsung_tag(std::span(o.service).first(o.service_len), o.samsung);
    output = o;
    return true;
}
bool parse_pwnagotchi(std::span<const uint8_t> data, std::span<char> name) {
    Json j;
    if (!j.parse({reinterpret_cast<const char *>(data.data()), data.size()}) ||
        j.tokens[0].type != Json::Object)
        return false;
    int64_t count{};
    if (!j.integer(j.member(0, "pwnd_tot"), count) || count < 0 || count > 0xffffffffLL)
        return false;
    if (!j.string(j.member(0, "name"), name) || !name[0])
        return false;
    for (char c : std::string_view(name.data()))
        if (c < 32 || c > 126)
            return false;
    char version[33];
    return j.string(j.member(0, "version"), version) && version[0];
}
bool parse_remote_id(std::span<const uint8_t> p, RemoteId &out) {
    if (p.size() != 29 || p[0] != 0xfa || p[1] != 0xff || p[2] != 0x0d)
        return false;
    auto m = p.subspan(4);
    RemoteId r{};
    r.type = m[0] >> 4;
    if ((m[0] & 15) != 2 || (r.type != 0 && r.type != 1 && r.type != 4))
        return false;
    auto coordinates = [&](int at) {
        r.latitude_e7 = le32s(m.data() + at);
        r.longitude_e7 = le32s(m.data() + at + 4);
        if (r.latitude_e7 < -900000000 || r.latitude_e7 > 900000000 ||
            r.longitude_e7 < -1800000000 || r.longitude_e7 > 1800000000)
            return false;
        r.location_valid = r.latitude_e7 != 0 || r.longitude_e7 != 0;
        return true;
    };
    if (r.type == 0) {
        r.id_type = m[1] >> 4;
        r.aircraft_type = m[1] & 15;
        if (r.id_type < 1 || r.id_type > 4)
            return false;
        std::copy_n(m.begin() + 2, 20, r.id.begin());
        if (std::all_of(r.id.begin(), r.id.end(), [](auto b) { return b == 0; }))
            return false;
        if (r.id_type <= 2) {
            bool ended = false;
            for (auto b : r.id) {
                if (!b)
                    ended = true;
                else if (ended || b < 32 || b > 126)
                    return false;
            }
        }
        if (m[22] || m[23] || m[24])
            return false;
    } else if (r.type == 1) {
        r.status = m[1] >> 4;
        if (r.status > 3 || (m[1] & 8) || m[2] > 180 || (m[23] & 0xf0) || m[24])
            return false;
        if (!coordinates(5))
            return false;
        auto time = le16(m.data() + 21);
        if (time != 0xffff && time > 36000)
            return false;
        auto alt = le16(m.data() + 15);
        r.altitude_valid = alt != 0;
        r.altitude_dm = int32_t(alt) * 5 - 10000;
    } else {
        if ((m[1] & 0xe0) || (m[1] & 3) > 2 || ((m[1] >> 2) & 7) > 1 || m[24])
            return false;
        if (!coordinates(2))
            return false;
        auto alt = le16(m.data() + 18);
        r.altitude_valid = alt != 0;
        r.altitude_dm = int32_t(alt) * 5 - 10000;
    }
    out = r;
    return true;
}
const char *grade(uint8_t score) {
    return score >= 80 ? "high" : score >= 50 ? "medium" : score >= 20 ? "low" : "informational";
}
const char *badge(uint8_t score) {
    return score >= 80 ? "STRONG MATCH" : score >= 50 ? "LIKELY" : "POSSIBLE";
}
} // namespace sniffer
