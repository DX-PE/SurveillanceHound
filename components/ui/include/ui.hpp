// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/appearance.hpp"
#include "sniffer/companion.hpp"
#include "sniffer/display.hpp"
#include "sniffer/follow.hpp"
#include "sniffer/tag_watch.hpp"
namespace sniffer::ui {
enum class Screen {
    Welcome,
    Calibration,
    Choose,
    Name,
    Privacy,
    Home,
    Log,
    Detectors,
    Settings,
    Diagnostics,
    Details,
    ResearchWarning,
    ResetWarning,
    Alerts,
    Clock,
    Battery,
    ClearWarning,
    SelfTest,
    ScentBook,
    ScentCard,
    Wardrobe,
    AlertActions,
    Ignored,
    Follow,
    Appearance,
    Display,
    TagWatch,
    WatchProgress
};
enum Request : uint32_t {
    Save = 1,
    Start = 2,
    Export = 4,
    Eject = 8,
    Demo = 16,
    Calibrate = 32,
    DiagnosticCopy = 64,
    Reset = 128,
    Rotate = 256,
    Pause = 512,
    ClearLogs = 1024,
    Sleep = 2048,
    Invert = 4096
};
enum class SnackPhase { None, Turn, Approach, Chew, Happy };
struct SnackPose {
    SnackPhase phase{SnackPhase::None};
    int frame{}, approach{}, bites{};
};
class View {
  public:
    View(Settings &s, Pet &p) : settings(s), pet(p) {}
    Settings &settings;
    Pet &pet;
    Screen screen{Screen::Welcome};
    Companion companion{}, preview_companion{};
    std::array<uint8_t, 32> identity_key{};
    uint64_t snooze_until{}, preview_snooze_until{}, unlock_until{};
    uint32_t preview_xp{};
    uint8_t unlocked_outfit{};
    int book_page{}, scent_index{}, scent_page{}, wardrobe_index{}, ignored_page{};
    Detection action_detection{};
    Companion &collection() {
        return demo ? preview_companion : companion;
    }
    const Companion &collection() const {
        return demo ? preview_companion : companion;
    }
    uint64_t snoozed_until() const {
        return demo ? preview_snooze_until : snooze_until;
    }
    uint64_t identity(const Detection &) const;
    bool alert_allowed(const Detection &) const;
    void reset_progress();
    void open_actions(const Detection &d);
    sniffer::Appearance appearance{}, preview_appearance{};
    sniffer::Appearance &look() {
        return demo ? preview_appearance : appearance;
    }
    const sniffer::Appearance &look() const {
        return demo ? preview_appearance : appearance;
    }
    uint32_t pets{};
    bool speaking() const {
        return look().speech && (happy_until > now ||
                                 (look().speech == 2 ? now % 12000 < 8000 : now % 24000 < 5000));
    }
    FollowScent follow{};
    sniffer::TagWatch tag_watch{};
    uint64_t watch_review{};
    unsigned watch_page{};
    bool watch_eligible(const sniffer::TagWatch::Entry &e) const {
        return (settings.enabled_categories & (1U << unsigned(e.category))) &&
               (settings.alert_categories & (1U << unsigned(e.category))) &&
               e.score >= std::max<uint8_t>(50, settings.threshold) &&
               !collection().is_ignored(e.hash);
    }
    void update_tag_watch() {
        if (tag_watch.demo != demo) {
            tag_watch.clear();
            watch_review = 0;
        }
        for (auto &e : tag_watch.entries)
            if (e.hash && (!e.fresh(now) || !watch_eligible(e) || !listening()))
                tag_watch.drop(e, !e.fresh(now));
        if (screen == Screen::TagWatch && !watch_review)
            if (const auto *warning = watch_warning())
                watch_review = warning->hash;
    }
    unsigned watch_count() const {
        unsigned count = 0;
        for (const auto &e : tag_watch.entries)
            count += e.fresh(now) && watch_eligible(e);
        return count;
    }
    const char *watch_hint() const {
        if (!tag_watch.armed)
            return "WATCH OFF / START IN CONTROLS";
        if (!listening())
            return "SNIFFING PAUSED / NO TIMING";
        if (snoozed_until() > now)
            return "ALERTS SNOOZED / RESUME IN CONTROLS";
        if (watch_count())
            return "SEEN = LAST SIGHTING / 2 MIN GAP RESTARTS";
        if (settings.threshold == 80)
            return "HIGH FILTER EXCLUDES SAMSUNG / FIND MY";
        for (size_t i = 0; i < recent_count; ++i) {
            const auto &d = recent[i];
            if (d.demo != demo || !TagWatch::tag(d.category) || now < d.last_ms ||
                now - d.last_ms > 90000)
                continue;
            if (!(settings.enabled_categories & (1U << unsigned(d.category))))
                return "RECENT TAG DETECTOR IS DISABLED";
            if (!(settings.alert_categories & (1U << unsigned(d.category))))
                return "RECENT TAG ALERT TYPE IS OFF";
            if (d.score < 50)
                return "RECENT TAG EVIDENCE BELOW MEDIUM";
        }
        if (collection().ignored_count())
            return "NO TIMERS / CHECK IGNORED SCENTS";
        return "WAITING FOR TAGS / NO MOVEMENT PROOF";
    }
    const sniffer::TagWatch::Entry *watch_warning() const {
        if (!tag_watch.armed || tag_watch.demo != demo || !listening() || snoozed_until() > now)
            return nullptr;
        for (const auto &e : tag_watch.entries)
            if (e.ready(now) && !e.acknowledged && watch_eligible(e))
                return &e;
        return nullptr;
    }
    bool watch_red() const {
        return settings.onboarded && screen != Screen::Calibration && screen != Screen::SelfTest &&
               watch_warning() &&
               (!tag_watch.flash || settings.reduced_animation || now % 2000 < 1000);
    }
    void open_tag_watch() {
        const auto *warning = watch_warning();
        watch_review = warning ? warning->hash : 0;
        watch_page = 0;
        screen = warning || !tag_watch.armed ? Screen::TagWatch : Screen::WatchProgress;
    }
    bool listening() const {
        return !paused && (demo || scanning);
    }
    ScentSignal signal() const {
        return follow.state(
            now, listening() && (settings.enabled_categories & (1U << unsigned(follow.category))));
    }
    void follow_scent(const Detection &d) {
        follow.select(d, now);
        alert_until = 0;
        screen = Screen::Follow;
    }
    void observe(const Observation &o, bool synthetic = false) {
        if (!paused && synthetic == demo && o.ms <= now &&
            (settings.enabled_categories & (1U << unsigned(follow.category))))
            follow.observe(o, synthetic);
    }
    uint32_t requests{};
    uint64_t now{}, alert_until{}, meal_until{}, happy_until{};
    Category meal_category{Category::FLIPPER};
    Detection alert_detection{}, detail_detection{};
    DroneTrack detail_drone{};
    bool detail_valid{};
    int detail_page{};
    bool demo{}, sd{}, sd_error{}, ble{}, scanning{}, paused{}, sd_read_only{};
    uint32_t wifi_accepted{}, ble_accepted{}, wifi_malformed{}, ble_malformed{}, radio_stack{},
        storage_stack{}, ble_stack{}, oversized{}, radio_errors{};
    uint64_t sd_free{}, sd_total{}, clock_seconds{}, clock_set_ms{}, level_until{};
    BatteryCalibration battery_cal{};
    int battery_millivolts{-1}, battery_level{-1}, battery_point{}, diagnostic_page{}, log_page{};
    std::array<char, 13> clock_input{};
    unsigned known_level{1}, self_test_taps{};
    DroneCache drones{};
    uint32_t heap{}, min_heap{}, wifi_count{}, ble_count{}, malformed{}, dropped{}, queue_high{},
        writes{}, boot_reason{}, stack_free{}, export_count{};
    int battery_adc{-1}, touch_x{}, touch_y{}, calibration_step{}, channel{}, detector_page{},
        settings_page{}, alert_page{};
    unsigned selected{};
    std::array<Detection, 32> recent{};
    size_t recent_count{};
    std::array<uint32_t, 3> counts{};
    std::array<char, 64> notice{};
    uint32_t largest_heap{};
    bool low_heap{};
    void memory_status(uint32_t free, uint32_t minimum, uint32_t largest) {
        heap = free;
        min_heap = minimum;
        largest_heap = largest;
        low_heap = free < 80000;
    }
    const char *home_notice() const {
        return notice[0] ? notice.data() : low_heap ? "LOW HEAP - SEE DIAGNOSTICS" : "";
    }
    int width() const {
        return display::width(settings.portrait);
    }
    int height() const {
        return display::height(settings.portrait);
    }
    int tile_rows() const {
        return settings.portrait ? 16 : 8;
    }
    static constexpr int max_tile_pixels = 320 * 16;
    static constexpr uint64_t meal_duration = 7000;
    SnackPose snack_pose() const;
    uint64_t utc_now() const {
        return clock_seconds && now >= clock_set_ms ? clock_seconds + (now - clock_set_ms) / 1000
                                                    : 0;
    }
    void rotate();
    void event(const Detection &);
    void tap(int x, int y);
    void next();
    void render(int tile_y, std::span<uint16_t> pixels);
};
} // namespace sniffer::ui
