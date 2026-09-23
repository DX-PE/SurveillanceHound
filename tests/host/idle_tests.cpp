// SPDX-License-Identifier: Apache-2.0
#include "ui.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace sniffer;
using namespace sniffer::ui;
namespace {
unsigned checks{};
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << "idle:" << __LINE__ << ": " #x "\n";                                      \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
std::vector<uint16_t> frame(View &v) {
    std::vector<uint16_t> pixels(v.width() * v.height() + 2, 0xabcd);
    for (int y = 0; y < v.height(); y += v.tile_rows())
        v.render(y, {pixels.data() + 1 + y * v.width(), size_t(v.width() * v.tile_rows())});
    CHECK(pixels.front() == 0xabcd && pixels.back() == 0xabcd);
    return pixels;
}
void timing() {
    Settings s;
    Pet p;
    View v(s, p);
    s.onboarded = true;
    v.scanning = v.sd = true;
    v.screen = Screen::Home;
    auto at = [&](uint64_t ms, DisplayMode wanted) {
        v.now = ms;
        v.update_display();
        CHECK(v.display_mode == wanted);
        CHECK(v.scanning && v.sd && !v.paused);
        CHECK(!(v.requests & (Pause | Sleep | Eject)));
    };
    at(119999, DisplayMode::Active);
    at(120000, DisplayMode::Dim);
    CHECK(v.display_brightness() == 15);
    at(299999, DisplayMode::Dim);
    at(300000, DisplayMode::Saver);
    CHECK(v.saver_since == 300000);
    at(899999, DisplayMode::Saver);
    at(900000, DisplayMode::Off);
    CHECK(v.display_brightness() == 0);
    const auto black = frame(v);
    CHECK(std::all_of(black.begin() + 1, black.end() - 1, [](auto pixel) { return pixel == 0; }));
    // A waking touch over the sniffing control cannot stop the radio.
    v.tap(330, 240);
    CHECK(v.display_mode == DisplayMode::Active && !v.paused && !(v.requests & Pause));
    CHECK(v.display_brightness() == s.brightness);
    at(1019999, DisplayMode::Active);
    at(1020000, DisplayMode::Dim);
    v.next();
    CHECK(v.screen == Screen::Home && v.display_mode == DisplayMode::Active);
    v.next();
    CHECK(v.screen == Screen::Log);
    // Armed Watch must block off, even when snoozed or sniffing is paused.
    v.wake_display();
    v.tag_watch.start(v.now, false);
    v.snooze_until = UINT64_MAX;
    at(4000000, DisplayMode::Saver);
    v.paused = true;
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Saver);
    v.paused = false;
    v.look().saver_minutes = 0;
    at(4000001, DisplayMode::Dim);
    v.look().dim_minutes = 0;
    at(4000002, DisplayMode::Active);
    v.tag_watch.clear();
    at(4000003, DisplayMode::Off);
    v.look().off_minutes = 0;
    at(4000004, DisplayMode::Active);
    v.look() = Appearance{};
    v.screen = Screen::Calibration;
    at(8000000, DisplayMode::Active);
    v.screen = Screen::SelfTest;
    at(9000000, DisplayMode::Active);
    v.screen = Screen::Home;
    s.onboarded = false;
    at(10000000, DisplayMode::Active);
    s.onboarded = true;
    v.export_status = ExportStatus::Exporting;
    at(11000000, DisplayMode::Active);
    v.export_status = ExportStatus::Complete;
    v.eject_status = EjectStatus::Ejecting;
    at(12000000, DisplayMode::Active);
    v.eject_status = EjectStatus::Safe;
    at(12000001, DisplayMode::Active);
    // Backwards clock does not unsigned-underflow into an idle state.
    at(1, DisplayMode::Active);
    v.preview_saver();
    CHECK(v.display_mode == DisplayMode::Saver);
    at(2, DisplayMode::Saver);
    v.tap(200, 150);
    CHECK(!v.saver_preview && v.display_mode == DisplayMode::Active && v.pets == 0);
}
void alerts() {
    Settings s;
    Pet p;
    View v(s, p);
    s.onboarded = true;
    v.scanning = true;
    v.identity_key.fill(42);
    Detection d{};
    d.category = Category::SAMSUNG_TAG;
    d.radio = Radio::Ble;
    d.score = 60;
    d.seen_count = 1;
    d.address = {0, 1, 2, 3, 4, 5};
    auto asleep = [&] {
        v.now += 1000000;
        v.screen = Screen::Log;
        v.update_display();
        CHECK(v.display_mode == DisplayMode::Off);
        d.first_ms = d.last_ms = v.now;
    };
    asleep();
    v.event(d);
    CHECK(v.display_mode == DisplayMode::Active && v.screen == Screen::Home &&
          v.alert_until > v.now);
    v.now += 4999;
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Active && v.last_activity == 0);
    ++v.now;
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Off); // Return immediately to the original idle stage.
    v.collection().ignore(v.identity(d), d.category);
    asleep();
    v.event(d);
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Off);
    v.collection().ignored = {};
    s.alert_categories = 0;
    asleep();
    v.event(d);
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Off);
    s.alert_categories = (1U << category_count) - 1;
    v.snooze_until = UINT64_MAX;
    asleep();
    v.event(d);
    CHECK(v.display_mode == DisplayMode::Off);
    v.snooze_until = 0;
    // Owner-connected SmartTags remain quiet.
    d.samsung.id[0] = 1;
    d.samsung.state = SamsungState::Connected;
    asleep();
    v.event(d);
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Off);
    // Watch can become eligible when a snooze expires without a new ordinary alert.
    v.tag_watch.start(v.now, false);
    d.samsung = {};
    const auto start = v.now;
    for (unsigned minute = 0; minute <= 10; ++minute) {
        v.now = start + minute * 60000;
        d.last_ms = v.now;
        v.tag_watch.observe(v.identity(d), d, v.now);
    }
    v.update_display();
    CHECK(v.watch_warning() && v.display_mode == DisplayMode::Active && v.screen == Screen::Home);
}
void frequent_alerts() {
    for (auto category : {Category::AIRTAG, Category::IBEACON}) {
        Settings s;
        Pet p;
        View v(s, p);
        s.onboarded = true;
        s.alert_categories |= 1U << unsigned(category);
        s.ibeacon_alerts = true;
        v.scanning = v.sd = true;
        v.screen = Screen::Home;
        v.now = 1000;
        v.wake_display();
        const auto activity = v.last_activity;
        Detection d{};
        d.category = category;
        d.radio = Radio::Ble;
        d.score = 85;
        d.seen_count = 1;
        d.meal = true;
        d.address = {0, 1, 2, 3, 4, 5};
        // Continuous cards and snacks every second must still leave 25 seconds
        // of each 30-second interval for the current idle stage.
        for (unsigned second = 0; second < 960; ++second) {
            v.now = activity + second * 1000;
            v.update_display();
            d.last_ms = v.now;
            v.event(d);
            v.update_display();
            CHECK(v.last_activity == activity);
            CHECK(v.scanning && v.sd && !v.paused && !(v.requests & (Pause | Sleep | Eject)));
            const auto expected = second < 120 || second % 30 < 5 ? DisplayMode::Active
                                  : second < 300                  ? DisplayMode::Dim
                                  : second < 900                  ? DisplayMode::Saver
                                                                  : DisplayMode::Off;
            CHECK(v.display_mode == expected);
            CHECK(v.alert_until > v.now && v.meal_until > v.now); // Events still reach the UI.
            if (expected == DisplayMode::Saver)
                CHECK(v.saver_since == activity + 300000); // Bounce path survives wake cycles.
        }
        CHECK(v.collection().scents[unsigned(category)].observations == 960);
        // A real touch during a brief alert wake restores the full idle timeout.
        v.now = activity + 960000;
        v.update_display();
        d.last_ms = v.now;
        v.event(d);
        v.update_display();
        CHECK(v.display_mode == DisplayMode::Active);
        v.tap(40, 110);
        CHECK(v.last_activity == v.now);
        v.now += 6000;
        v.update_display();
        CHECK(v.display_mode == DisplayMode::Active);
    }
    // A following warning takes precedence even during an ordinary wake cooldown.
    Settings s;
    Pet p;
    View v(s, p);
    s.onboarded = true;
    v.scanning = true;
    v.now = 1000000;
    v.update_display();
    Detection d{};
    d.category = Category::AIRTAG;
    d.radio = Radio::Ble;
    d.score = 85;
    d.last_ms = v.now;
    v.event(d);
    v.now += 6000;
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Off);
    v.tag_watch.start(v.now - TagWatch::duration, false);
    v.tag_watch.entries[0] = {42,   v.now - TagWatch::duration, v.now, 11, Category::AIRTAG, 85,
                              false};
    v.update_display();
    CHECK(v.watch_warning() && v.display_mode == DisplayMode::Active);
    for (unsigned second = 0; second < 360; ++second) {
        v.now += 1000;
        v.tag_watch.entries[0].last = v.now;
        v.update_display();
        CHECK(v.display_mode == DisplayMode::Active);
    }
    v.tag_watch.entries[0].acknowledged = true;
    v.now += 300000;
    v.update_display();
    CHECK(v.display_mode == DisplayMode::Saver && v.tag_watch.armed);
}
void screens() {
    for (bool portrait : {false, true}) {
        Settings s;
        Pet p;
        View v(s, p);
        s.onboarded = true;
        s.portrait = portrait;
        v.screen = Screen::Display;
        const int top = portrait ? 80 : 66, step = portrait ? 55 : 34;
        v.tap(30, top + 4 * step + 10);
        CHECK(v.screen == Screen::IdleDisplay);
        for (int row = 0; row < 3; ++row) {
            auto &value = row == 0   ? v.look().dim_minutes
                          : row == 1 ? v.look().saver_minutes
                                     : v.look().off_minutes;
            const auto initial = value;
            for (unsigned i = 0; i < std::size(idle_timeouts); ++i) {
                v.requests = 0;
                v.tap(30, top + row * step + 10);
                CHECK(valid_timeout(value) && (v.requests & Save));
                frame(v);
            }
            CHECK(value == initial);
        }
        v.demo = true;
        v.tap(30, top + 10);
        CHECK(v.preview_appearance.dim_minutes == 5 && v.appearance.dim_minutes == 2);
        v.tap(30, top + 3 * step + 10);
        CHECK(v.saver_preview && v.display_mode == DisplayMode::Saver);
        v.tap(30, top + 3 * step + 10);
        CHECK(!v.saver_preview && v.screen == Screen::IdleDisplay);
        v.preview_saver();
        std::vector<uint16_t> previous;
        for (unsigned breed = 0; breed < 6; ++breed) {
            s.character = breed;
            auto current = frame(v);
            if (breed)
                CHECK(current != previous);
            previous = current;
        }
        v.collection().equipped = 4;
        CHECK(frame(v) != previous);
        // Entire sprite stays inside either orientation, including reflections.
        for (uint64_t elapsed = 0; elapsed < 3600000; elapsed += 137) {
            v.now = v.saver_since + elapsed;
            const auto pos = v.saver_position();
            CHECK(pos.x >= 8 && pos.y >= 8 && pos.x + 128 <= v.width() - 8 &&
                  pos.y + 128 <= v.height() - 8);
        }
        v.now = v.saver_since;
        auto a = v.saver_position();
        v.now += 400;
        auto b = v.saver_position();
        CHECK(b.x > a.x && b.y > a.y);
        v.now = v.saver_since + uint64_t(v.width() - 144 - 37) * 40;
        a = v.saver_position();
        v.now += 40;
        b = v.saver_position();
        CHECK(a.x == v.width() - 136 && b.x == a.x - 1);
        s.reduced_animation = true;
        v.now = v.saver_since;
        auto still = frame(v);
        v.now += 14999;
        CHECK(still == frame(v));
        ++v.now;
        CHECK(still != frame(v));
    }
}
} // namespace
int main() {
    timing();
    alerts();
    frequent_alerts();
    screens();
    std::cout << checks << " idle display checks passed\n";
}
