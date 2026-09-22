// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "ui.hpp"
#include <fstream>
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ui_preview output.ppm\n";
        return 1;
    }
    sniffer::Settings settings;
    settings.onboarded = true;
    sniffer::Pet pet;
    sniffer::ui::View view(settings, pet);
    view.screen = sniffer::ui::Screen::Home;
    view.now = 7000;
    view.sd = true;
    view.scanning = true;
    std::ofstream out(argv[1], std::ios::binary);
    out << "P6\n1440 640\n255\n";
    // Six actual firmware screens, rendered in tiles to a host-only contact sheet.
    for (int panel_row = 0; panel_row < 2; ++panel_row)
        for (int y = 0; y < 320; y += 8) {
            std::array<std::array<uint16_t, 480 * 8>, 3> tiles;
            for (int panel_col = 0; panel_col < 3; ++panel_col) {
                int index = panel_row * 3 + panel_col;
                view.screen = index < 3    ? sniffer::ui::Screen::Home
                              : index == 3 ? sniffer::ui::Screen::Choose
                              : index == 4 ? sniffer::ui::Screen::Detectors
                                           : sniffer::ui::Screen::Settings;
                settings.character = index;
                view.render(y, tiles[panel_col]);
            }
            for (int line = 0; line < 8; ++line)
                for (int col = 0; col < 3; ++col)
                    for (int x = 0; x < 480; ++x) {
                        auto p = tiles[col][line * 480 + x];
                        char rgb[] = {char(((p >> 11) & 31) * 255 / 31),
                                      char(((p >> 5) & 63) * 255 / 63), char((p & 31) * 255 / 31)};
                        out.write(rgb, 3);
                    }
        }
    return out ? 0 : 1;
}
