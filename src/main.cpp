#include "demo.h"
#include "plots.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace {
void top_down(Rectangle a, const radar::Config& cfg, const radar::Demo& demo) {
    const auto pos = [&](double range) { return Vector2{a.x + a.width / 2, a.y + a.height * static_cast<float>(1 - range / cfg.max_range())}; };
    DrawLineV(pos(0), pos(cfg.max_range()), ui::muted);
    DrawCircleV(pos(0), 6, ui::cyan);
    for (const auto& t : demo.frame().truth) {
        const auto p = pos(t.range_m);
        DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y), 10, ui::cyan);
    }
    for (const auto& d : demo.frame().detections) {
        const auto p = pos(d.range_m);
        DrawLineEx({p.x - 5, p.y - 5}, {p.x + 5, p.y + 5}, 2, ui::gold);
        DrawLineEx({p.x + 5, p.y - 5}, {p.x - 5, p.y + 5}, 2, ui::gold);
    }
    for (const auto& t : demo.tracks()) {
        if (!t.confirmed()) continue;
        const auto p = pos(t.range_m);
        DrawRectangleLinesEx({p.x - 7, p.y - 7, 14, 14}, 2, t.misses ? ui::muted : GREEN);
        char label[80];
        std::snprintf(label, sizeof(label), "#%d  %.1f m  %+.1f m/s%s", t.id, t.range_m, t.velocity_mps, t.misses ? " coast" : "");
        DrawText(label, static_cast<int>(p.x + 18), static_cast<int>(p.y - 6), 13, ui::ink);
    }
    char label[40];
    for (int i = 0; i <= 4; ++i) {
        std::snprintf(label, sizeof(label), "%.0f m", cfg.max_range() * i / 4);
        DrawText(label, static_cast<int>(a.x - 46), static_cast<int>(a.y + a.height * (1 - i / 4.0) - 6), 13, ui::muted);
    }
    DrawText("lateral x = 0 m / radar at origin", static_cast<int>(a.x + 30), static_cast<int>(a.y + a.height + 14), 13, ui::muted);
}
}

int main() {
    try {
        radar::Config cfg;
        radar::CfarConfig cfar_cfg;
        int count = 3;
        radar::Demo demo(cfg, radar::default_targets(cfg, count));
        demo.step(cfg, cfar_cfg);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
        InitWindow(1440, 960, "FMCW radar | signal to track");
        SetWindowMinSize(1200, 850);
        SetTargetFPS(60);
        bool paused = false;
        double accumulator = 0;
        while (!WindowShouldClose()) {
            if (IsKeyPressed(KEY_SPACE)) paused = !paused;
            bool reset = IsKeyPressed(KEY_R);
            const float w = static_cast<float>(GetScreenWidth()), h = static_cast<float>(GetScreenHeight());
            const float side = 258, gap = 16, x = side + gap;
            const float col = (w - x - 3 * gap) / 2;
            const float row = (h - 150) / 2;
            BeginDrawing();
            ClearBackground(ui::background);
            DrawText("FMCW / SIGNAL TO TRACK", 24, 20, 28, ui::ink);
            char status[180];
            std::snprintf(status, sizeof(status), "77 GHz  |  %d samples x %d chirps  |  t = %.2f s  |  %s", cfg.samples(), cfg.chirps, demo.frame().time_s, paused ? "PAUSED" : "LIVE");
            DrawText(status, 25, 56, 16, ui::muted);
            DrawRectangleRounded({16, 94, side - 16, h - 114}, 0.04f, 6, ui::panel);
            DrawText("LIVE CONTROLS", 32, 112, 18, ui::ink);
            ui::slider({32, 154, 208, 50}, "SNR / unit echo", cfg.snr_db, -35, 40, "%.1f dB");
            double target_count = count;
            if (ui::slider({32, 220, 208, 50}, "Targets", target_count, 1, 3, "%.0f")) {
                const int selected = static_cast<int>(std::round(target_count));
                if (selected != count) { count = selected; reset = true; }
            }
            ui::slider({32, 286, 208, 50}, "CFAR threshold", cfar_cfg.threshold_factor, 2, 40, "%.1fx");
            if (ui::button({32, 353, 208, 39}, cfg.window ? "Hann window: ON" : "Hann window: OFF")) cfg.window = !cfg.window;
            if (ui::button({32, 406, 208, 39}, paused ? "Resume [Space]" : "Pause [Space]")) paused = !paused;
            if (ui::button({32, 459, 208, 39}, "Reset scene [R]")) reset = true;
            std::snprintf(status, sizeof(status), "Range bin     %.3f m\nVelocity bin  %.3f m/s\nRange limit   %.1f m\nSpeed limit   +/-%.1f m/s", cfg.range_resolution(), cfg.velocity_resolution(), cfg.max_range(), cfg.max_velocity());
            DrawText(status, 32, 526, 16, ui::muted);
            DrawText("TOP-DOWN LEGEND", 32, 636, 16, ui::ink);
            DrawText("O  Ground truth", 32, 666, 16, ui::cyan);
            DrawText("X  CFAR measurement", 32, 694, 16, ui::gold);
            DrawText("[] Kalman estimate", 32, 722, 16, GREEN);
            DrawText("Grey = prediction only", 32, 750, 15, ui::muted);

            if (reset) { demo = radar::Demo(cfg, radar::default_targets(cfg, count)); demo.step(cfg, cfar_cfg); accumulator = 0; }
            if (!paused) accumulator += GetFrameTime();
            if (accumulator >= cfg.frame_s) { demo.step(cfg, cfar_cfg); accumulator = 0; }
            const auto& frame = demo.frame();
            std::vector<double> raw_i, raw_q, spectrum;
            double raw_max = 1;
            for (auto v : frame.beat[0]) { raw_i.push_back(v.real()); raw_q.push_back(v.imag()); raw_max = std::max({raw_max, std::abs(v.real()), std::abs(v.imag())}); }
            for (auto v : frame.range[0]) spectrum.push_back(20 * std::log10(std::max(1e-6, std::abs(v))));
            auto a = ui::chart({x, 94, col, row}, "01 / Raw beat signal", "First chirp | I: cyan, Q: amber | amplitude");
            ui::line(a, raw_q, -raw_max, raw_max, ui::gold);
            ui::line(a, raw_i, -raw_max, raw_max, ui::cyan);
            ui::axes(a, (cfg.samples() - 1) / cfg.sample_rate_hz * 1e6, "time / us", -raw_max, raw_max);
            a = ui::chart({x + col + gap, 94, col, row}, "02 / Range spectrum", "First chirp | dB amplitude, unit echo = 0 dB");
            ui::line(a, spectrum, -80, 20, ui::cyan);
            ui::axes(a, (cfg.range_bins() - 1) * cfg.range_resolution(), "range / m", -80, 20);
            a = ui::chart({x, 94 + row + gap, col, row}, "03 / Range-Doppler + CFAR", "Velocity / m/s | positive = receding | circles = CFAR");
            ui::heatmap(a, cfg, frame.map);
            for (const auto& d : frame.detections)
                DrawCircleLines(static_cast<int>(a.x + a.width * (d.range_bin + 0.5) / cfg.range_bins()),
                    static_cast<int>(a.y + a.height * (cfg.chirps - d.doppler_bin - 0.5) / cfg.chirps), 5, WHITE);
            a = ui::chart({x + col + gap, 94 + row + gap, col, row}, "04 / Top-down tracks", "Radial scene | single RX: no angle estimate");
            top_down(a, cfg, demo);
            const int confirmed = static_cast<int>(std::count_if(demo.tracks().begin(), demo.tracks().end(), [](const auto& t) { return t.confirmed(); }));
            std::snprintf(status, sizeof(status), "CFAR peaks: %zu  |  confirmed tracks: %d  |  heatmap: -70 to 0 dB (fixed)  |  %d FPS", frame.detections.size(), confirmed, GetFPS());
            DrawText(status, static_cast<int>(x), static_cast<int>(h - 25), 14, ui::muted);
            EndDrawing();
        }
        CloseWindow();
    } catch (const std::exception& e) { std::cerr << "radar_demo: " << e.what() << '\n'; return 1; }
}
