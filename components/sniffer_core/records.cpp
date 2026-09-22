// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include "sniffer/json.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace sniffer {
namespace {
class Writer {
  public:
    explicit Writer(std::span<char> out) : out_(out) {}
    void text(std::string_view s) {
        if (!ok_ || s.size() >= out_.size() - used_) {
            ok_ = false;
            return;
        }
        std::copy(s.begin(), s.end(), out_.begin() + used_);
        used_ += s.size();
        out_[used_] = 0;
    }
    void number(int64_t n) {
        char b[32];
        std::snprintf(b, sizeof(b), "%lld", static_cast<long long>(n));
        text(b);
    }
    void string(std::string_view s) {
        text("\"");
        for (unsigned char c : s) {
            if (c == '"')
                text("\\\"");
            else if (c == '\\')
                text("\\\\");
            else if (c < 32 || c > 126) {
                char b[7];
                std::snprintf(b, sizeof(b), "\\u%04x", c);
                text(b);
            } else {
                char b = char(c);
                text({&b, 1});
            }
        }
        text("\"");
    }
    size_t size() const {
        return ok_ ? used_ : 0;
    }

  private:
    std::span<char> out_;
    size_t used_{};
    bool ok_{true};
};
} // namespace
size_t json_escape(std::string_view s, std::span<char> output) {
    if (output.empty())
        return 0;
    Writer w(output);
    w.string(s);
    return w.size();
}
size_t write_record(const Detection &d, std::string_view session, std::string_view token,
                    bool research, std::span<char> output, uint8_t region) {
    if (output.empty() || d.demo || !d.rule_count || d.rule_count > d.rules.size() ||
        static_cast<size_t>(d.category) >= category_count || !d.seen_count)
        return 0;
    Writer w(output);
    w.text("{\"schema_version\":1,\"firmware_version\":");
    w.string(firmware_version);
    w.text(",\"hardware\":\"E32R40T\",\"signature_pack\":");
    w.string(signature_pack_version);
    w.text(",\"region\":");
    w.string(region == 0 ? "US" : "EU");
    w.text(",\"event_id\":");
    char id[96];
    std::snprintf(id, sizeof(id), "%.*s-%llu-%u-%u", static_cast<int>(session.size()),
                  session.data(), static_cast<unsigned long long>(d.last_ms), unsigned(d.category),
                  unsigned(d.seen_count));
    w.string(id);
    w.text(",\"session_id\":");
    w.string(session);
    w.text(",\"observed_at\":");
    char utc[24]{};
    format_utc(d.unix_seconds, utc);
    if (utc[0])
        w.string(utc);
    else
        w.text("null");
    w.text(",\"time_quality\":");
    w.string(utc[0] ? "manual" : "uptime_only");
    w.text(",\"uptime_ms\":");
    w.number(d.last_ms);
    w.text(",\"first_seen_ms\":");
    w.number(d.first_ms);
    w.text(",\"category\":");
    w.string(categories[static_cast<size_t>(d.category)]);
    w.text(",\"label\":");
    w.string(d.rules[0]->label);
    w.text(",\"confidence\":");
    w.string(grade(d.score));
    w.text(",\"score\":");
    w.number(d.score);
    w.text(",\"rule_ids\":[");
    for (size_t i = 0; i < d.rule_count; ++i) {
        if (i)
            w.text(",");
        w.string(d.rules[i]->id);
    }
    w.text("],\"reason\":");
    w.string(d.rules[0]->reason);
    w.text(",\"radio\":");
    w.string(d.radio == Radio::Wifi ? "wifi" : "ble");
    w.text(",\"channel\":");
    w.number(d.channel);
    w.text(",\"rssi_min\":");
    w.number(d.rssi_min);
    w.text(",\"rssi_max\":");
    w.number(d.rssi_max);
    w.text(",\"rssi_avg\":");
    w.number(d.rssi_sum / d.seen_count);
    w.text(",\"seen_count\":");
    w.number(d.seen_count);
    w.text(",\"identifier\":");
    w.string(token);
    w.text(",\"address_type\":");
    w.string(d.address_type ? "random" : "public");
    w.text(",\"meal_awarded\":");
    w.text(d.meal ? "true" : "false");
    w.text(",\"privacy_mode\":");
    w.string(research ? "research" : "private");
    if (research) {
        char mac[18];
        std::snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", d.address[0], d.address[1],
                      d.address[2], d.address[3], d.address[4], d.address[5]);
        w.text(",\"raw_mac\":");
        w.string(mac);
    }
    w.text("}\n");
    return w.size();
}
size_t write_export_record(const Detection &d, std::string_view session, std::string_view token,
                           bool research, std::span<char> output, uint8_t region) {
    if (output.empty() || d.demo || !d.rule_count || d.rule_count > d.rules.size() ||
        static_cast<size_t>(d.category) >= category_count || !d.seen_count)
        return 0;
    Writer w(output);
    w.text("{\"schema_version\":1,\"firmware_version\":");
    w.string(firmware_version);
    w.text(",\"hardware\":\"E32R40T\",\"signature_pack\":");
    w.string(signature_pack_version);
    w.text(",\"session_id\":");
    w.string(session);
    w.text(",\"identifier\":");
    w.string(token);
    w.text(",\"category\":");
    w.string(categories[size_t(d.category)]);
    w.text(",\"rule_ids\":[");
    for (size_t i = 0; i < d.rule_count; ++i) {
        if (i)
            w.text(",");
        w.string(d.rules[i]->id);
    }
    w.text("]");
    w.text(",\"first_seen_ms\":");
    w.number(d.first_ms);
    w.text(",\"last_seen_ms\":");
    w.number(d.last_ms);
    w.text(",\"seen_count\":");
    w.number(d.seen_count);
    w.text(",\"score\":");
    w.number(d.score);
    w.text(",\"confidence\":");
    w.string(grade(d.score));
    w.text(",\"rssi_min\":");
    w.number(d.rssi_min);
    w.text(",\"rssi_max\":");
    w.number(d.rssi_max);
    w.text(",\"channel\":");
    w.number(d.channel);
    w.text(",\"radio\":");
    w.string(d.radio == Radio::Wifi ? "wifi" : "ble");
    w.text(",\"region\":");
    w.string(region == 0 ? "US" : "EU");
    w.text(",\"source_privacy_mode\":");
    w.string(research ? "research" : "private");
    w.text(",\"privacy_mode\":\"sanitized\",\"time_quality\":");
    char utc[24]{};
    format_utc(d.unix_seconds, utc);
    w.string(utc[0] ? "manual" : "uptime_only");
    w.text(",\"observed_at\":");
    if (utc[0])
        w.string(utc);
    else
        w.text("null");
    w.text(",");
    w.text("\"latitude\":null,\"longitude\":null,\"location_source\":\"none\"}\n");
    return w.size();
}

namespace {
bool validate_log(const Json &j) {
    if (j.tokens[0].type != Json::Object)
        return false;
    auto str = [&](const char *key, std::span<char> out) {
        return j.string(j.member(0, key), out);
    };
    auto number = [&](const char *key, int64_t lo, int64_t hi) {
        int64_t n;
        return j.integer(j.member(0, key), n) && n >= lo && n <= hi;
    };
    char b[160];
    for (auto key : {"event_id", "firmware_version", "hardware", "identifier", "label", "reason",
                     "session_id", "signature_pack"})
        if (!str(key, b) || !b[0])
            return false;
    if (!str("hardware", b) || std::string_view(b) != "E32R40T")
        return false;
    if (!str("session_id", b) || std::strlen(b) != 32 ||
        std::string_view(b).find_first_not_of("0123456789abcdef") != std::string_view::npos)
        return false;
    if (!str("identifier", b) || std::strlen(b) != 20 ||
        std::string_view(b).find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567") !=
            std::string_view::npos)
        return false;
    if (!str("category", b))
        return false;
    bool known = false;
    for (auto c : categories)
        known |= std::string_view(b) == c;
    if (!known)
        return false;
    if (!number("schema_version", 1, 1) || !number("score", 1, 100) ||
        !number("seen_count", 1, 0xffffffffLL) || !number("channel", 0, 14))
        return false;
    for (auto key : {"first_seen_ms", "uptime_ms"})
        if (!number(key, 0, 9007199254740991LL))
            return false;
    int64_t first, last, score;
    j.integer(j.member(0, "first_seen_ms"), first);
    j.integer(j.member(0, "uptime_ms"), last);
    j.integer(j.member(0, "score"), score);
    if (first > last || !str("confidence", b) || std::string_view(b) != grade(score))
        return false;
    for (auto key : {"rssi_min", "rssi_max", "rssi_avg"})
        if (!number(key, -127, 20))
            return false;
    int64_t low, high, avg;
    j.integer(j.member(0, "rssi_min"), low);
    j.integer(j.member(0, "rssi_max"), high);
    j.integer(j.member(0, "rssi_avg"), avg);
    if (low > avg || avg > high)
        return false;
    for (auto key : {"radio", "region", "privacy_mode", "address_type", "time_quality"}) {
        if (!str(key, b))
            return false;
        std::string_view v(b), k(key);
        if (k == "radio" && v != "wifi" && v != "ble")
            return false;
        if (k == "region" && v != "US" && v != "EU")
            return false;
        if (k == "privacy_mode" && v != "private" && v != "research")
            return false;
        if (k == "address_type" && v != "public" && v != "random")
            return false;
        if (k == "time_quality" && v != "uptime_only" && v != "manual")
            return false;
    }
    int time = j.member(0, "observed_at");
    str("time_quality", b);
    if (std::string_view(b) == "uptime_only") {
        if (j.raw(time) != "null")
            return false;
    } else {
        char utc[24];
        if (!j.string(time, utc) || std::strlen(utc) != 20 || utc[4] != '-' || utc[7] != '-' ||
            utc[10] != 'T' || utc[13] != ':' || utc[16] != ':' || utc[19] != 'Z')
            return false;
        char digits[13];
        std::snprintf(digits, sizeof(digits), "%.4s%.2s%.2s%.2s%.2s", utc, utc + 5, utc + 8,
                      utc + 11, utc + 14);
        uint64_t n;
        if (!parse_utc(digits, n) || utc[17] < '0' || utc[17] > '5' || utc[18] < '0' ||
            utc[18] > '9')
            return false;
    }
    int meal = j.member(0, "meal_awarded");
    if (meal < 0 || j.tokens[meal].type != Json::Boolean)
        return false;
    int rules = j.member(0, "rule_ids");
    if (rules < 0 || j.tokens[rules].type != Json::Array || j.tokens[rules].after - rules < 2 ||
        j.tokens[rules].after - rules > 5)
        return false;
    for (int i = rules + 1; i < j.tokens[rules].after; ++i) {
        char id[65];
        if (!j.string(i, id) || !id[0])
            return false;
        for (char c : std::string_view(id))
            if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_' && c != '.')
                return false;
    }
    return true;
}
} // namespace
bool valid_log_record(std::string_view line) {
    Json j;
    return j.parse(line) && validate_log(j);
}
size_t export_log_record(std::string_view line, std::span<const uint8_t> key,
                         std::span<char> output) {
    Json j;
    if (!j.parse(line) || !validate_log(j) || output.empty() || key.size() != 32)
        return 0;
    char session[33], id[21], identity[56];
    j.string(j.member(0, "session_id"), session);
    j.string(j.member(0, "identifier"), id);
    std::snprintf(identity, sizeof(identity), "%s:%s", session, id);
    std::array<uint8_t, 32> digest{};
    if (!hmac_sha256(key, {reinterpret_cast<const uint8_t *>(identity), std::strlen(identity)},
                     digest))
        return 0;
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    unsigned bits = 0, value = 0, k = 0;
    for (auto b : digest) {
        value = (value << 8) | b;
        bits += 8;
        while (bits >= 5 && k < 20) {
            bits -= 5;
            id[k++] = alphabet[(value >> bits) & 31];
        }
        if (k == 20)
            break;
    }
    id[20] = 0;
    if (!hmac_sha256(key, {reinterpret_cast<const uint8_t *>(session), std::strlen(session)},
                     digest))
        return 0;
    for (size_t i = 0; i < 16; ++i)
        std::snprintf(session + i * 2, 3, "%02x", digest[i]);
    Writer w(output);
    w.text("{");
    bool comma = false;
    for (auto field :
         {"schema_version", "firmware_version", "hardware", "signature_pack", "session_id",
          "category", "rule_ids", "first_seen_ms", "seen_count", "score", "confidence", "rssi_min",
          "rssi_max", "channel", "radio", "region", "time_quality", "observed_at"}) {
        if (comma)
            w.text(",");
        comma = true;
        w.string(field);
        w.text(":");
        if (std::string_view(field) == "session_id")
            w.string(session);
        else
            w.text(j.raw(j.member(0, field)));
    }
    w.text(",\"identifier\":");
    w.string(id);
    w.text(",\"last_seen_ms\":");
    w.text(j.raw(j.member(0, "uptime_ms")));
    w.text(",\"source_privacy_mode\":");
    w.text(j.raw(j.member(0, "privacy_mode")));
    w.text(",\"privacy_mode\":\"sanitized\",\"latitude\":null,\"longitude\":null,\"location_"
           "source\":\"none\"}\n");
    return w.size();
}
} // namespace sniffer
