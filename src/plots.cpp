#include "plots.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace ui {
Rectangle chart(Rectangle outer, const std::string& title, const std::string& subtitle) {
    DrawRectangleRounded(outer, 0.04f, 6, panel);
    DrawText(title.c_str(), static_cast<int>(outer.x + 18), static_cast<int>(outer.y + 14), 20, ink);
    DrawText(subtitle.c_str(), static_cast<int>(outer.x + 18), static_cast<int>(outer.y + 39), 14, muted);
    Rectangle area{outer.x + 58, outer.y + 72, outer.width - 80, outer.height - 108};
    for (int i = 0; i <= 4; ++i) {
        const float x = area.x + area.width * i / 4;
        const float y = area.y + area.height * i / 4;
        DrawLineV({x, area.y}, {x, area.y + area.height}, Color{40, 54, 70, 255});
        DrawLineV({area.x, y}, {area.x + area.width, y}, Color{40, 54, 70, 255});
    }
    return area;
}
void line(Rectangle a, const std::vector<double>& values, double low, double high, Color color) {
    if (values.size() < 2 || high <= low) return;
    for (std::size_t i = 1; i < values.size(); ++i) {
        const auto point = [&](std::size_t k) {
            return Vector2{a.x + a.width * static_cast<float>(k) / (values.size() - 1),
                a.y + a.height * static_cast<float>(1 - std::clamp((values[k] - low) / (high - low), 0.0, 1.0))};
        };
        DrawLineEx(point(i - 1), point(i), 1.5f, color);
    }
}
void axes(Rectangle a, double xmax, const char* unit, double ymin, double ymax) {
    char text[80];
    for (int i = 0; i <= 4; ++i) {
        std::snprintf(text, sizeof(text), "%.1f", xmax * i / 4);
        DrawText(text, static_cast<int>(a.x + a.width * i / 4 - 10), static_cast<int>(a.y + a.height + 9), 13, muted);
    }
    std::snprintf(text, sizeof(text), "%.1f", ymax);
    DrawText(text, static_cast<int>(a.x - 50), static_cast<int>(a.y), 13, muted);
    std::snprintf(text, sizeof(text), "%.1f", ymin);
    DrawText(text, static_cast<int>(a.x - 50), static_cast<int>(a.y + a.height - 13), 13, muted);
    DrawText(unit, static_cast<int>(a.x + a.width / 2 - 12), static_cast<int>(a.y + a.height + 22), 12, muted);
}
bool slider(Rectangle a, const char* title, double& value, double low, double high, const char* format) {
    char label[100];
    std::snprintf(label, sizeof(label), format, value);
    DrawText(title, static_cast<int>(a.x), static_cast<int>(a.y), 16, muted);
    DrawText(label, static_cast<int>(a.x + a.width - MeasureText(label, 16)), static_cast<int>(a.y), 16, ink);
    Rectangle track{a.x, a.y + 31, a.width, 5};
    const double previous = value;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), {a.x - 8, a.y + 20, a.width + 16, 26}))
        value = low + std::clamp((GetMouseX() - a.x) / a.width, 0.0f, 1.0f) * (high - low);
    DrawRectangleRec(track, {49, 67, 86, 255});
    const float x = a.x + a.width * static_cast<float>((value - low) / (high - low));
    DrawRectangleRec({a.x, track.y, x - a.x, track.height}, cyan);
    DrawCircleV({x, track.y + 2}, 7, cyan);
    return previous != value;
}
bool button(Rectangle a, const char* title) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), a);
    DrawRectangleRounded(a, 0.15f, 5, hover ? Color{50, 76, 94, 255} : Color{32, 51, 68, 255});
    DrawText(title, static_cast<int>(a.x + 12), static_cast<int>(a.y + 10), 16, ink);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
Color heat_color(double db) {
    const double t = std::clamp((db + 70) / 70, 0.0, 1.0);
    const Color stops[] = {{13, 22, 43, 255}, {32, 63, 127, 255}, {19, 156, 173, 255}, {247, 215, 96, 255}};
    const int k = std::min(2, static_cast<int>(t * 3));
    const double f = t * 3 - k;
    return {static_cast<unsigned char>(stops[k].r * (1 - f) + stops[k + 1].r * f),
            static_cast<unsigned char>(stops[k].g * (1 - f) + stops[k + 1].g * f),
            static_cast<unsigned char>(stops[k].b * (1 - f) + stops[k + 1].b * f), 255};
}
void heatmap(Rectangle a, const radar::Config& cfg, const radar::RangeDoppler& map) {
    for (int d = 0; d < cfg.chirps; ++d)
        for (int r = 0; r < cfg.range_bins(); ++r) {
            const double db = 10 * std::log10(std::max(1e-12, map.power[d][r]));
            DrawRectangleRec({a.x + a.width * r / cfg.range_bins(),
                a.y + a.height * (cfg.chirps - 1 - d) / cfg.chirps,
                a.width / cfg.range_bins() + 0.5f, a.height / cfg.chirps + 0.5f}, heat_color(db));
        }
    axes(a, cfg.max_range(), "range / m", -cfg.max_velocity(), cfg.max_velocity());
}
}
