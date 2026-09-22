// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace sniffer {
constexpr char firmware_version[] = "0.1.0-dev";
constexpr size_t category_count = 19;
enum class Category : uint8_t {
    FLOCK,
    AXON,
    GLASSES,
    SKIMMER,
    RAVEN,
    AIRTAG,
    TILE,
    SAMSUNG_TAG,
    GOOGLE_TAG,
    DRONE,
    ALPR,
    CAMERA,
    RING,
    IBEACON,
    FLIPPER,
    PWNAGOTCHI,
    PINEAPPLE,
    DEAUTH,
    EVIL_TWIN
};
constexpr const char *categories[] = {
    "FLOCK",       "AXON",       "GLASSES",   "SKIMMER", "RAVEN",    "AIRTAG", "TILE",
    "SAMSUNG_TAG", "GOOGLE_TAG", "DRONE",     "ALPR",    "CAMERA",   "RING",   "IBEACON",
    "FLIPPER",     "PWNAGOTCHI", "PINEAPPLE", "DEAUTH",  "EVIL_TWIN"};
constexpr const char *foods[] = {
    "plate biscuit", "camera biscuit", "glasses chew",    "card biscuit", "microphone cookie",
    "round tag",     "square tag",     "diamond tag",     "map pin",      "drone snack",
    "plate biscuit", "camera biscuit", "doorbell cookie", "beacon berry", "dolphin",
    "pixel ghost",   "pineapple",      "static snack",    "twin mask"};
enum class Radio : uint8_t { Wifi, Ble };
enum class Kind : uint8_t {
    Oui,
    SsidExact,
    SsidPrefix,
    NameExact,
    NamePrefix,
    Uuid16,
    Uuid128,
    Company,
    Payload,
    Protocol,
    Behavior,
    Count
};
constexpr size_t kind_count = static_cast<size_t>(Kind::Count);
using Address = std::array<uint8_t, 6>;
struct Observation {
    uint64_t ms{};
    Address address{};
    Radio radio{};
    uint8_t address_type{}, channel{}, subtype{};
    int8_t rssi{};
    bool protected_network{}, rsn{}, contradiction{};
    uint16_t rsn_caps{};
    std::array<uint8_t, 32> ssid{};
    uint8_t ssid_len{};
    std::array<char, 33> name{};
    std::array<uint16_t, 8> uuids{};
    uint8_t uuid_count{}, uuid128_count{};
    std::array<std::array<uint8_t, 16>, 1> uuids128{};
    bool pwnagotchi{};
    uint32_t vendor_fingerprint{};
    std::array<uint8_t, 31> manufacturer{}, service{};
    uint8_t manufacturer_len{}, service_len{};
    std::array<uint8_t, 128> vendor{};
    uint8_t vendor_len{};
};
struct Rule {
    const char *id;
    Category category;
    Kind kind;
    const char *value;
    uint8_t score, cap;
    bool generic;
    const char *label;
    const char *reason;
};
struct Detection {
    Category category{};
    Radio radio{};
    Address address{};
    uint8_t address_type{}, channel{}, score{};
    uint64_t first_ms{}, last_ms{};
    int8_t rssi_min{}, rssi_max{};
    int64_t rssi_sum{};
    uint32_t seen_count{};
    std::array<const Rule *, 4> rules{};
    uint8_t rule_count{};
    uint64_t unix_seconds{};
    bool meal{}, demo{};
};
struct Settings {
    uint32_t version{2};
    uint32_t enabled_categories{(1U << category_count) - 1U};
    uint8_t threshold{50}, region{0}, brightness{70}, character{0};
    bool sound{}, low_feeding{}, reduced_animation{}, research{}, raw_ssid{}, onboarded{};
    bool ibeacon_alerts{};
    std::array<char, 13> name{'S', 'C', 'O', 'U', 'T', 0};
    bool portrait{}, rotation_locked{true};
    uint32_t alert_categories{((1U << category_count) - 1U) & ~(1U << unsigned(Category::IBEACON))};
};
inline bool should_alert(const Settings &s, const Detection &d) {
    return unsigned(d.category) < category_count &&
           (s.alert_categories & (1U << unsigned(d.category))) && d.score >= s.threshold;
}
struct Pet {
    uint32_t xp{}, meals{}, unique{}, category_mask{};
    std::array<uint32_t, category_count> lifetime_meals{};
    uint8_t fullness{70}, mood{70};
    uint64_t decay_ms{};
    unsigned level() const {
        return 1 + xp / 100;
    }
    void tick(uint64_t now);
    void feed(const Detection &, bool low_feeding = false);
    void stroke();
};
struct MealStamp {
    uint64_t hash{}, remaining_ms{};
};
class MealGuard {
  public:
    std::array<MealStamp, 128> stamps{};
    bool award(uint64_t hash, uint64_t now, Category category, uint8_t score, bool low);
    void restore(std::span<const MealStamp> saved);
    std::array<MealStamp, 32> snapshot(uint64_t now) const;

  private:
    std::array<uint64_t, category_count> category_until_{};
    uint64_t global_until_{};
};
class Engine {
  public:
    explicit Engine(std::span<const Rule> rules) : rules_(rules) {}
    size_t ingest(const Observation &, const Settings &, std::span<Detection> out);
    // One count per radio/address/type seen within 90 seconds, at its highest
    // current confidence. Reuses the bounded evidence cache; these are signals,
    // not people.
    std::array<uint32_t, 3> recent_counts(uint64_t now, const Settings &) const;

  private:
    struct Evidence {
        const Rule *rule{};
        uint32_t at{};
    };
    struct Entry {
        bool used{};
        Detection detection{};
        uint64_t emitted{};
        std::array<Evidence, kind_count> evidence{};
    };
    struct Burst {
        bool used{};
        Address address{};
        uint8_t channel{};
        std::array<uint64_t, 20> times{};
        uint8_t count{}, next{};
        uint64_t last{};
    };
    struct AccessPoint {
        bool used{};
        struct Summary {
            uint64_t ms{};
            Address address{};
            std::array<uint8_t, 32> ssid{};
            uint8_t ssid_len{};
            bool protected_network{};
            uint16_t rsn_caps{};
            int8_t rssi{};
            uint8_t channel{};
            uint32_t vendor_fingerprint{};
        } obs{};
    };
    std::span<const Rule> rules_;
    std::array<Entry, 128> cache_{};
    std::array<Burst, 16> bursts_{};
    std::array<AccessPoint, 24> aps_{};
    uint64_t next_emit_{};
    bool behavior(const Rule &, const Observation &);
    void remember_ap(const Observation &);
};
bool parse_wifi(std::span<const uint8_t>, Observation &);
bool parse_ble(std::span<const uint8_t>, Observation &);
bool matches(const Rule &, const Observation &);
void sanitize(std::span<const uint8_t>, std::span<char>);
const char *grade(uint8_t score);
const char *badge(uint8_t score);
struct RemoteId {
    uint8_t type{};
    int32_t latitude_e7{}, longitude_e7{};
    bool location_valid{};
    std::array<uint8_t, 20> id{};
    uint8_t id_type{}, aircraft_type{}, status{};
    int32_t altitude_dm{};
    bool altitude_valid{};
};
bool parse_remote_id(std::span<const uint8_t>, RemoteId &);
struct DroneTrack {
    Address address{};
    uint8_t address_type{}, present{};
    std::array<uint64_t, 3> updated{};
    RemoteId basic{}, location{}, system{};
};
class DroneCache {
  public:
    void ingest(const Observation &);
    const DroneTrack *find(const Address &, uint8_t address_type, uint64_t now);

  private:
    std::array<DroneTrack, 8> tracks_{};
};
bool parse_pwnagotchi(std::span<const uint8_t>, std::span<char> name);
struct BatteryCalibration {
    std::array<uint16_t, 3> adc{};
    std::array<uint16_t, 3> millivolts{3300, 3700, 4200};
    bool enabled{};
};
bool valid_battery(const BatteryCalibration &);
int battery_mv(const BatteryCalibration &, int adc);
int battery_percent(int millivolts);
bool parse_utc(std::string_view digits, uint64_t &seconds);
void format_utc(uint64_t seconds, std::span<char> out);
bool valid_log_record(std::string_view);
size_t export_log_record(std::string_view, std::span<const uint8_t> key, std::span<char> out);
bool hmac_sha256(std::span<const uint8_t> key, std::span<const uint8_t> input,
                 std::span<uint8_t, 32> output);
bool sha256(std::span<const uint8_t> input, std::span<uint8_t, 32> output);
bool private_token(std::span<const uint8_t> key, const Detection &, std::span<char, 21> output);
uint64_t meal_hash(std::span<const uint8_t> key, const Detection &);
// Returns zero on insufficient space. No partial JSON record may be persisted.
size_t write_record(const Detection &, std::string_view session, std::string_view token,
                    bool research, std::span<char> output, uint8_t region = 0);
size_t write_export_record(const Detection &, std::string_view session, std::string_view token,
                           bool research, std::span<char> output, uint8_t region = 0);
size_t json_escape(std::string_view, std::span<char>);
} // namespace sniffer
