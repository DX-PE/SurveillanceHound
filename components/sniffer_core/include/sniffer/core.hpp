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
// A supported Samsung finding advertisement. The broadcast ID can itself rotate.
// Connection state is an unauthenticated claim, never proof of ownership.
enum class SamsungState : uint8_t {
    Unavailable,
    Unknown,
    RecentlySeparated,
    Offline,
    LongOffline,
    Connected
};
struct SamsungTag {
    std::array<uint8_t, 8> id{};
    SamsungState state{SamsungState::Unavailable};
    bool valid() const {
        return state != SamsungState::Unavailable;
    }
};
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
    SamsungTag samsung{};
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
// Volatile, field-serialized event data: group aligned members to avoid padding in
// every cache/history entry. This is not a persisted NVS or on-card binary layout.
struct Detection {
    uint64_t first_ms{}, last_ms{}, unix_seconds{};
    int64_t rssi_sum{};
    std::array<const Rule *, 4> rules{};
    uint32_t seen_count{};
    SamsungTag samsung{};
    Address address{};
    Category category{};
    Radio radio{};
    uint8_t address_type{}, channel{}, score{}, rule_count{};
    int8_t rssi_min{}, rssi_max{};
    bool meal{}, demo{};
};
static_assert(sizeof(Detection) <= (sizeof(void *) == 4 ? 80 : 96),
              "Keep volatile detection metadata within its RAM budget");
inline bool samsung_identity(const Detection &d) {
    return d.category == Category::SAMSUNG_TAG && d.radio == Radio::Ble && d.samsung.valid();
}
inline bool samsung_connected(const Detection &d) {
    return samsung_identity(d) && d.samsung.state == SamsungState::Connected;
}
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
           (s.alert_categories & (1U << unsigned(d.category))) && d.score >= s.threshold &&
           !samsung_connected(d);
}
struct Pet {
    static constexpr uint64_t needs_interval_ms = 300000;
    static constexpr uint8_t low_needs = 25;
    static void decay_needs(uint64_t now, uint64_t &clock, uint8_t &fullness, uint8_t &mood);
    static void nourish(uint8_t score, uint8_t &fullness, uint8_t &mood);
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
    // One count per identity (Samsung broadcast ID, otherwise radio/address/type)
    // seen within 90 seconds, at its highest
    // current confidence. Reuses the bounded evidence cache; these are signals,
    // not people.
    std::array<uint32_t, 3> recent_counts(uint64_t now, const Settings &) const;

  private:
    struct Evidence {
        const Rule *rule{};
        uint32_t at{};
    };
    // Exact/prefix SSID and name rules share evidence groups. Keep those groups
    // dense rather than reserving unused slots in every cached identity.
    static constexpr size_t evidence_group_count = kind_count - 2;
    struct Entry {
        Detection detection{}; // seen_count == 0 marks an unused entry.
        uint64_t emitted{};
        SamsungState emitted_samsung_state{};
        std::array<Evidence, evidence_group_count> evidence{};
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
// Input includes the little-endian service UUID. Unsupported formats leave output untouched.
bool parse_samsung_tag(std::span<const uint8_t>, SamsungTag &);
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
// Legacy address identity, retained to honor existing saved ignores.
uint64_t address_hash(std::span<const uint8_t> key, const Detection &);
uint64_t meal_hash(std::span<const uint8_t> key, const Detection &);
// Returns zero on insufficient space. No partial JSON record may be persisted.
size_t write_record(const Detection &, std::string_view session, std::string_view token,
                    bool research, std::span<char> output, uint8_t region = 0);
size_t write_export_record(const Detection &, std::string_view session, std::string_view token,
                           bool research, std::span<char> output, uint8_t region = 0);
size_t json_escape(std::string_view, std::span<char>);
} // namespace sniffer
