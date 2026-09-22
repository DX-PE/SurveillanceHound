// SPDX-License-Identifier: Apache-2.0
#pragma once
namespace sniffer::display {
struct Point {
    int x, y;
};
constexpr int width(bool portrait) {
    return portrait ? 320 : 480;
}
constexpr int height(bool portrait) {
    return portrait ? 480 : 320;
}
// Calibration stays in the panel's original portrait coordinate space.
constexpr Point from_portrait(Point p, bool portrait) {
    return portrait ? p : Point{p.y, 319 - p.x};
}
constexpr Point to_portrait(Point p, bool portrait) {
    return portrait ? p : Point{319 - p.y, p.x};
}
constexpr Point target(int i, bool portrait) {
    constexpr Point tall[] = {{30, 160}, {289, 180}, {160, 390}};
    constexpr Point wide[] = {{30, 110}, {449, 110}, {240, 240}};
    return portrait ? tall[i] : wide[i];
}
} // namespace sniffer::display
