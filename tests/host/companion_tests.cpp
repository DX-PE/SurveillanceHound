// SPDX-License-Identifier: Apache-2.0
#include "assets_generated.h"
#include "signatures_generated.h"
#include "state.hpp"
#include "ui.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace sniffer;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "companion:%d: %s\n", __LINE__, #x);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

void visible_airtag_target_tests() {
    for (bool portrait : {false, true}) {
        Settings s;
        s.onboarded = true;
        s.portrait = portrait;
        Pet pet;
        ui::View v(s, pet);
        v.identity_key[0] = 17;
        v.screen = ui::Screen::Home;
        v.scanning = true;
        Engine engine(production_rules);
        auto sighting = [&](uint8_t id, uint64_t ms) {
            Observation o{};
            o.ms = v.now = ms;
            o.address = {0xC2, 1, 2, 3, 4, id};
            o.address_type = 1;
            o.rssi = -45;
            std::array<uint8_t, 31> ad{30, 0xff, 0x4c, 0, 0x12, 0x19};
            ad[7] = id;
            CHECK(parse_ble(ad, o));
            std::array<Detection, 4> found{};
            CHECK(engine.ingest(o, s, found) == 1);
            CHECK(found[0].category == Category::AIRTAG && found[0].score == 75);
            v.event(found[0]);
            return found[0];
        };
        const auto seen = sighting(1, 1000);
        std::vector<uint16_t> tile(v.width() * v.tile_rows());
        for (int y = 0; y < v.height(); y += v.tile_rows())
            v.render(y, tile);
        // Production processes received radio events before the next touch, so the
        // backing alert can change while the previously drawn card is still visible.
        const auto later = sighting(2, 1200);
        CHECK(v.identity(seen) != v.identity(later));
        v.tap(portrait ? 190 : 320, portrait ? 375 : 198);
        CHECK(v.screen == ui::Screen::AlertActions);
        CHECK(v.identity(v.action_detection) == v.identity(seen));
        v.now += 200;
        v.event(later); // Another arrival while the frozen action menu is open.
        for (int y = 0; y < v.height(); y += v.tile_rows())
            v.render(y, tile);
        CHECK(v.displayed_identity == v.identity(seen) && !v.displayed_ignored);
        v.tap(30, (portrait ? 80 : 66) + 3 * (portrait ? 55 : 34) + 12);
        CHECK(v.is_ignored(seen) && !v.is_ignored(later));
        // The unrelated current alert remains eligible. Repeated packets from the
        // ignored Apple identity stay quiet without hiding other Find My signals.
        CHECK(v.alert_until > v.now && v.alert_allowed(later));
        v.alert_until = 0;
        for (unsigned cycle = 1; cycle <= 120; ++cycle) {
            auto repeat = sighting(1, 1000 + cycle * 30000);
            CHECK(v.is_ignored(repeat) && !v.alert_until);
        }
        CHECK(v.companion.ignored_count() == 1);
        auto other = sighting(2, v.now + 200);
        CHECK(!v.is_ignored(other) && v.alert_until > v.now);
        CHECK(engine.recent_counts(v.now, s)[1] == 2);
        // Reopening an ignored sighting reports that exact saved state.
        v.open_actions(seen);
        for (int y = 0; y < v.height(); y += v.tile_rows())
            v.render(y, tile);
        CHECK(v.displayed_ignored && v.displayed_identity == v.identity(seen));
        // Save/reload preserves the Apple ignore without depending on SD.
        ui::View reloaded(s, pet);
        reloaded.identity_key = v.identity_key;
        reloaded.companion = v.companion;
        reloaded.now = v.now + 30000;
        reloaded.event(seen);
        CHECK(reloaded.is_ignored(seen) && !reloaded.alert_until);
        reloaded.event(other);
        CHECK(reloaded.alert_until > reloaded.now);
        // A changed address or address type remains a distinct current identity.
        auto changed = seen;
        changed.address_type = 0;
        CHECK(!reloaded.is_ignored(changed));
        changed = seen;
        changed.address.back() = 3;
        CHECK(!reloaded.is_ignored(changed));
        changed = seen;
        changed.demo = true;
        CHECK(!reloaded.is_ignored(changed));
        // Expiring/replacing the backing alert cannot retarget the still visible button.
        v.screen = ui::Screen::Home;
        v.event(other);
        for (int y = 0; y < v.height(); y += v.tile_rows())
            v.render(y, tile);
        const auto shown_code = v.displayed_identity;
        v.now += 6000;
        changed = other;
        changed.address.back() = 4;
        v.event(changed);
        v.tap(portrait ? 190 : 320, portrait ? 375 : 198);
        CHECK(v.screen == ui::Screen::AlertActions);
        CHECK(v.identity(v.action_detection) == shown_code);
        // After a frame with no card, no invisible Ignore button is active.
        v.screen = ui::Screen::Home;
        v.alert_until = 0;
        for (int y = 0; y < v.height(); y += v.tile_rows())
            v.render(y, tile);
        v.event(changed);
        v.tap(portrait ? 190 : 320, portrait ? 375 : 198);
        CHECK(v.screen == ui::Screen::Home);
    }
}

void ignore_rearrival_tests() {
    for (bool portrait : {false, true}) {
        Settings settings;
        settings.onboarded = true;
        settings.portrait = portrait;
        Pet pet;
        ui::View view(settings, pet);
        view.identity_key[0] = 17;
        view.now = 1000;
        view.scanning = true;
        view.screen = ui::Screen::Home;
        Detection tag{};
        tag.category = Category::SAMSUNG_TAG;
        tag.radio = Radio::Ble;
        tag.address_type = 1;
        tag.address = {0xC2, 1, 2, 3, 4, 5};
        tag.score = 60;
        tag.meal = true;
        const int ignore_y = (portrait ? 80 : 66) + 3 * (portrait ? 55 : 34) + 12;
        // A repeat can arrive while Quiet Alerts is open and arm an unseen Home card.
        view.event(tag);
        view.open_actions(tag);
        view.now += 30000;
        view.event(tag);
        CHECK(view.alert_until > view.now);
        view.tap(30, ignore_y);
        CHECK(view.companion.ignored_count() == 1);
        CHECK(view.alert_until == 0); // Ignore must remove the already-pending card too.
        CHECK(view.meal_until > view.now);
        CHECK(!view.tag_watch.armed); // Ordinary alerts, without Travel Watch.
        const auto observations = view.companion.scents[size_t(tag.category)].observations;
        for (int i = 0; i < 40; ++i) {
            view.now += 30000;
            tag.last_ms = view.now;
            tag.seen_count++;
            tag.rssi_max = int8_t(-40 - i);
            view.event(tag);
            CHECK(!view.alert_until && !view.alert_allowed(tag));
        }
        CHECK(view.companion.ignored_count() == 1);
        CHECK(view.companion.scents[size_t(tag.category)].observations == observations + 40);
        // A different tag can still alert, including one that arrives during Ignore.
        auto other = tag;
        other.address.back()++;
        view.open_actions(tag);
        view.event(other);
        view.tap(30, ignore_y);
        CHECK(view.alert_until > view.now && view.alert_allowed(other));
        CHECK(view.companion.ignored_count() == 1);
        // Re-ignoring the selected identity is idempotent, and clears its stale card.
        view.open_actions(tag);
        view.alert_detection = tag;
        view.alert_until = view.now + 5000;
        view.tap(30, ignore_y);
        CHECK(!view.alert_until && view.companion.ignored_count() == 1);
        // Both tags stay quiet after separate ignores; log/meal processing continues.
        view.open_actions(other);
        view.event(other);
        view.tap(30, ignore_y);
        CHECK(!view.alert_until && view.companion.ignored_count() == 2);
        for (int i = 0; i < 40; ++i) {
            view.now += 30000;
            view.event(tag);
            view.event(other);
            CHECK(!view.alert_until && view.meal_until > view.now);
        }
    }
}

void ignore_identity_lifetime_tests() {
    for (bool portrait : {false, true}) {
        Settings s;
        s.onboarded = true;
        s.portrait = portrait;
        Pet pet;
        ui::View v(s, pet);
        v.identity_key[0] = 17;
        v.scanning = true;
        Engine engine(production_rules);
        std::array<Address, 2> addresses{{{0xC2, 1, 2, 3, 4, 5}, {0xC2, 1, 2, 3, 4, 6}}};
        auto sighting = [&](Address address, uint64_t ms) {
            Observation o{};
            o.ms = v.now = ms;
            o.address = address;
            o.address_type = 1;
            o.rssi = -45;
            constexpr uint8_t ad[] = {2, 1, 6, 3, 3, 0x5a, 0xfd};
            CHECK(parse_ble(ad, o));
            std::array<Detection, 4> found{};
            CHECK(engine.ingest(o, s, found) == 1);
            CHECK(found[0].category == Category::SAMSUNG_TAG && found[0].score == 60);
            v.event(found[0]);
            return found[0];
        };
        for (unsigned i = 0; i < addresses.size(); ++i) {
            auto d = sighting(addresses[i], 1000 + i * 200);
            v.open_actions(d);
            v.tap(30, (portrait ? 80 : 66) + 3 * (portrait ? 55 : 34) + 12);
            CHECK(v.companion.is_ignored(v.identity(d)) && !v.alert_until);
        }
        // Ignore has no uptime expiry, even when the classifier's evidence ages out.
        for (unsigned hour = 1; hour <= 24; ++hour) {
            for (unsigned i = 0; i < addresses.size(); ++i) {
                sighting(addresses[i], uint64_t(hour) * 3600000 + i * 200);
                CHECK(!v.alert_until && !v.tag_watch.armed);
            }
            CHECK(engine.recent_counts(v.now, s)[1] == 2);
        }
        // Two new addresses can produce alerts while the visible total remains two.
        const auto changed_at = v.now + 91000;
        for (unsigned i = 0; i < addresses.size(); ++i) {
            addresses[i].back() += 16;
            auto d = sighting(addresses[i], changed_at + i * 200);
            CHECK(!v.companion.is_ignored(v.identity(d)) && v.alert_until > v.now);
        }
        CHECK(engine.recent_counts(v.now, s)[1] == 2);
        CHECK(v.companion.ignored_count() == 2);
        // The existing category toggle covers new addresses, but also excludes Watch.
        v.screen = ui::Screen::Alerts;
        v.alert_page = unsigned(Category::SAMSUNG_TAG) / 4;
        v.tap(30, (portrait ? 80 : 64) + 3 * (portrait ? 68 : 43) + 12);
        CHECK(!(s.alert_categories & (1U << unsigned(Category::SAMSUNG_TAG))));
        CHECK(s.enabled_categories & (1U << unsigned(Category::SAMSUNG_TAG)));
        CHECK(!v.alert_until && (v.requests & ui::Save));
        v.tag_watch.start(v.now, false);
        const auto observations = v.companion.scents[size_t(Category::SAMSUNG_TAG)].observations;
        const auto watch_start = v.now;
        for (unsigned minute = 1; minute <= 12; ++minute) {
            for (unsigned i = 0; i < addresses.size(); ++i)
                sighting(addresses[i], watch_start + minute * 60000 + i * 200);
            CHECK(!v.alert_until && !v.watch_warning() && !v.watch_count());
            CHECK(engine.recent_counts(v.now, s)[1] == 2);
        }
        CHECK(v.companion.scents[size_t(Category::SAMSUNG_TAG)].observations == observations + 24);
        auto unrelated = v.recent[0];
        unrelated.category = Category::AIRTAG;
        CHECK(v.alert_allowed(unrelated));
    }
}

int main() {
    visible_airtag_target_tests();
    ignore_rearrival_tests();
    ignore_identity_lifetime_tests();
    for (bool portrait : {false, true}) {
        Settings s;
        s.onboarded = true;
        s.portrait = portrait;
        Pet pet;
        ui::View v(s, pet);
        v.identity_key[0] = 17;
        v.now = 1000;
        v.screen = ui::Screen::Home;
        Detection d{};
        d.category = Category::FLIPPER;
        d.address = {2, 1, 2, 3, 4, 5};
        d.radio = Radio::Ble;
        d.score = 85;
        d.meal = true;
        d.unix_seconds = 1790035200;
        d.rssi_max = -42;
        d.rules[0] = &production_rules[0];
        d.rule_count = 1;
        pet.xp = 25;
        v.event(d);
        CHECK(v.companion.discoveries() == 1 && v.preview_companion.discoveries() == 0);
        CHECK(v.companion.scents[size_t(d.category)].first_utc == d.unix_seconds);
        CHECK(v.companion.unlocked == 3 && v.unlock_until > v.now);
        CHECK(v.requests & ui::Save);
        // The on-alert action target is frozen even while detections continue.
        v.tap(portrait ? 210 : 360, portrait ? 373 : 201);
        CHECK(v.screen == ui::Screen::AlertActions && !v.alert_until);
        auto another = d;
        another.address[5] = 6;
        v.event(another);
        CHECK(v.action_detection.address == d.address);
        const int row_y = portrait ? 80 : 66, step = portrait ? 55 : 34;
        v.tap(30, row_y + 3 * step + 12);
        CHECK(v.screen == ui::Screen::Home && v.companion.ignored_count() == 1);
        CHECK(!v.alert_allowed(d) && v.alert_allowed(another));
        auto before = v.companion.scents[size_t(d.category)].observations;
        v.alert_until = 0;
        v.event(d);
        CHECK(!v.alert_until && v.meal_until > v.now);
        CHECK(v.companion.scents[size_t(d.category)].observations == before + 1);
        CHECK(v.recent[0].address == d.address);
        auto different = d;
        different.category = Category::PINEAPPLE;
        CHECK(v.alert_allowed(different));
        different = d;
        different.address_type = 1;
        CHECK(v.alert_allowed(different));
        different = d;
        different.radio = Radio::Wifi;
        CHECK(v.alert_allowed(different));
        // Removal restores only the selected ignored device/category.
        v.screen = ui::Screen::Ignored;
        v.tap(30, (portrait ? 80 : 64) + 12);
        CHECK(!v.companion.ignored_count() && v.alert_allowed(d));
        for (int duration = 0; duration < 3; ++duration) {
            constexpr uint64_t minutes[] = {5, 15, 60};
            v.open_actions(d);
            v.tap(30, row_y + duration * step + 12);
            CHECK(v.snooze_until == v.now + minutes[duration] * 60000);
            CHECK(!v.alert_allowed(d) && !v.alert_allowed(another));
            v.now = v.snooze_until - 1;
            CHECK(!v.alert_allowed(d));
            ++v.now;
            CHECK(v.alert_allowed(d));
        }
        v.open_actions(d);
        v.tap(30, row_y + 12);
        v.screen = ui::Screen::Settings;
        v.settings_page = 0;
        v.tap(30, row_y + 4 * step + 12);
        CHECK(!v.snooze_until && v.alert_allowed(d));
        s.alert_categories &= ~(1U << unsigned(d.category));
        CHECK(!v.alert_allowed(d));
        s.alert_categories |= 1U << unsigned(d.category);
        s.threshold = 80;
        auto low = d;
        low.score = 50;
        CHECK(!v.alert_allowed(low));
        // Book navigation reaches all categories, including the final partial page.
        v.screen = ui::Screen::Home;
        v.tap(v.width() - 45, 44);
        CHECK(v.screen == ui::Screen::ScentBook);
        for (int page = 0; page < 4; ++page) {
            v.book_page = page;
            v.tap(30, portrait ? 95 : 80);
            CHECK(v.screen == ui::Screen::ScentCard && v.scent_index == page * 6);
            v.tap(v.width() - 30, v.height() - 65);
            CHECK(v.scent_page == 1);
            v.tap(30, v.height() - 65);
            CHECK(v.screen == ui::Screen::ScentBook);
        }
        // Locked previews cannot be equipped; accumulated XP unlocks permanent gear.
        v.screen = ui::Screen::Wardrobe;
        v.wardrobe_index = 4;
        v.tap(v.width() - 30, v.height() - 65);
        CHECK(v.companion.equipped == 0);
        pet.xp = 300;
        v.event(d);
        v.tap(v.width() - 30, v.height() - 65);
        CHECK(v.companion.equipped == 4 && (v.requests & ui::Save));
        for (unsigned i = 0; i < 10; ++i) {
            auto sighting = d;
            sighting.category = Category(i);
            v.event(sighting);
        }
        CHECK(v.companion.unlocked == 63);
        // Synthetic discoveries, ignores, rewards and snooze never modify the real collection.
        auto real = v.companion;
        auto real_xp = pet.xp;
        v.demo = true;
        auto fake = d;
        fake.demo = true;
        v.event(fake);
        CHECK(v.preview_companion.discoveries() == 1 && v.preview_xp == 25);
        v.open_actions(fake);
        v.tap(30, row_y + 3 * step + 12);
        CHECK(v.preview_companion.ignored_count() == 1 && !v.companion.ignored_count());
        v.open_actions(fake);
        v.tap(30, row_y + 12);
        CHECK(v.preview_snooze_until > v.now && !v.snooze_until);
        CHECK(v.companion.scents[size_t(d.category)].observations ==
              real.scents[size_t(d.category)].observations);
        CHECK(pet.xp == real_xp && v.companion.equipped == real.equipped);
        v.reset_progress();
        CHECK(!v.preview_xp && !v.preview_companion.discoveries() && v.companion.discoveries());
        CHECK(v.preview_companion.ignored_count() == 1);
        v.demo = false;
        // Every new screen, category, outfit, breed and animation frame renders inside its tile.
        std::vector<uint16_t> guarded(ui::View::max_tile_pixels + 2, 0xabcd);
        auto render = [&] {
            for (int y = 0; y < v.height(); y += v.tile_rows())
                v.render(y, {guarded.data() + 1, size_t(v.width() * v.tile_rows())});
            CHECK(guarded.front() == 0xabcd && guarded.back() == 0xabcd);
        };
        for (int screen = int(ui::Screen::ScentBook); screen <= int(ui::Screen::Ignored);
             ++screen) {
            v.screen = ui::Screen(screen);
            for (int page = 0; page < 4; ++page) {
                v.book_page = v.ignored_page = page;
                render();
            }
        }
        v.screen = ui::Screen::ScentCard;
        for (int category = 0; category < int(category_count); ++category)
            for (int page = 0; page < 3; ++page) {
                v.scent_index = category;
                v.scent_page = page;
                render();
            }
        for (int dog = 0; dog < 6; ++dog) {
            s.character = dog;
            for (unsigned outfit = 0; outfit < outfit_count; ++outfit) {
                v.companion.equipped = outfit;
                v.wardrobe_index = outfit;
                v.screen = ui::Screen::Wardrobe;
                render();
                v.screen = ui::Screen::Home;
                v.now = 10000000;
                v.alert_until = 0;
                v.meal_until = v.now + ui::View::meal_duration;
                for (int phase : {0, 1500, 2700, 3500, 4400, 5400, 6000}) {
                    v.now = 10000000 + phase;
                    render();
                }
                v.paused = true;
                v.meal_until = 0;
                render();
                v.paused = false;
            }
        }
    }
    Companion c;
    for (unsigned i = 1; i <= c.ignored.size(); ++i)
        CHECK(c.ignore(i, Category::FLOCK));
    CHECK(c.ignore(1, Category::FLOCK));
    CHECK(!c.ignore(999, Category::AXON) && c.ignored_count() == ignore_capacity &&
          c.is_ignored(1));
    CHECK(!c.ignore(0, Category::FLOCK));
    CHECK(valid_companion(c));
    c.equipped = 5;
    CHECK(!valid_companion(c));
    c.unlocked = 63;
    CHECK(valid_companion(c));
    c.ignored[1] = c.ignored[0];
    CHECK(!valid_companion(c));
    // The expanded list is paged over occupied slots, including the last slot;
    // removing its only row returns to the preceding page rather than stranding it.
    for (bool portrait : {false, true}) {
        Settings settings;
        settings.onboarded = true;
        settings.portrait = portrait;
        Pet pet;
        ui::View view(settings, pet);
        view.screen = ui::Screen::Ignored;
        for (unsigned i = 1; i <= ignore_capacity; ++i)
            CHECK(view.companion.ignore(i, Category::AIRTAG));
        CHECK(view.ignored_pages() == 16 && view.ignored_slot(63) == 63);
        view.ignored_page = 15;
        for (unsigned i = 0; i < 4; ++i)
            view.tap(20, portrait ? 90 : 70);
        CHECK(view.companion.ignored_count() == 60 && view.ignored_page == 14);
        CHECK(view.ignored_slot(60) == ignore_capacity);
        view.tap(view.width() - 50, view.height() - 30);
        CHECK(view.screen == ui::Screen::IgnoreBackup);
        view.requests = 0;
        view.tap(20, portrait ? 90 : 70);
        CHECK(view.requests & ui::BackupIgnores);
        CHECK(view.backup_pending);
        view.backup_queued(false);
        CHECK(std::string_view(view.notice.data()) == "BACKUP PENDING / AUTO RETRY");
        view.requests = 0;
        view.tap(20, portrait ? 90 : 70);
        CHECK(!view.requests); // Repeated taps cannot fill both snapshot slots.
        view.ignore_backup = IgnoreBackupStatus::Checking;
        view.backup_progress(0);
        CHECK(view.backup_pending);
        view.ignore_backup = IgnoreBackupStatus::Saved;
        view.backup_progress(1);
        CHECK(!view.backup_pending && std::string_view(view.notice.data()) == "SD BACKUP SAVED");
        view.sd = true;
        view.sd_checking = true;
        view.request_export();
        CHECK(view.export_status == ui::ExportStatus::Checking && !view.requests);
        view.requests = 0;
        view.demo = true;
        view.tap(20, portrait ? 90 : 70);
        CHECK(!view.requests);
        CHECK(std::string_view(view.ignore_backup_message()) == "DEMO / NO SD WRITES");
    }
    storage::StateV2 legacy;
    legacy.meal_key[0] = 1;
    legacy.pet.xp = 320;
    legacy.settings.research = true;
    storage::State upgraded;
    CHECK(storage::migrate(legacy, upgraded));
    upgraded.companion.unlock(upgraded.pet.xp);
    CHECK(upgraded.version == 5 && storage::valid_current(upgraded));
    CHECK(upgraded.settings.research && upgraded.companion.unlocked == 23);
    CHECK(!upgraded.companion.discoveries());
    upgraded.companion.equipped = 6;
    CHECK(!storage::valid_current(upgraded));
    std::printf("%u companion checks passed\n", checks);
}
