#include "simulation.h"
#include "range_fft.h"
#include "doppler_fft.h"
#include "cfar.h"
#include "plots.h"
#include <cmath>

int main() {
    radar::Config cfg;
    radar::Simulator sim;
    radar::CfarConfig cfar_cfg;
    InitWindow(1100, 880, "FMCW | Range-Doppler processing");
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        const auto beat = sim.beat(cfg, {{25, 3, 1}, {50, -2, 0.8}});
        const auto range = radar::range_fft(cfg, beat);
        const auto map = radar::doppler_fft(cfg, range);
        const auto detections = radar::cfar(cfg, map, cfar_cfg);
        std::vector<double> raw, spectrum;
        for (auto v : beat[0]) raw.push_back(v.real());
        for (auto v : range[0]) spectrum.push_back(20 * std::log10(std::max(1e-6, std::abs(v))));
        BeginDrawing();
        ClearBackground(ui::background);
        DrawText("FMCW RADAR / range processing", 24, 22, 26, ui::ink);
        ui::slider({30, 70, 360, 50}, "SNR", cfg.snr_db, -30, 40, "%.1f dB");
        ui::slider({580, 70, 360, 50}, "CFAR factor", cfar_cfg.threshold_factor, 2, 40, "%.1fx");
        auto a = ui::chart({20, 150, 520, 300}, "01 / Beat signal", "First chirp - I component");
        ui::line(a, raw, -4, 4, ui::cyan);
        ui::axes(a, cfg.chirp_s * 1e6, "us", -4, 4);
        a = ui::chart({560, 150, 520, 300}, "02 / Range spectrum", "Hann + radix-2 FFT - dB amplitude");
        ui::line(a, spectrum, -80, 5, ui::gold);
        ui::axes(a, (cfg.range_bins() - 1) * cfg.range_resolution(), "m", -80, 5);
        a = ui::chart({20, 465, 1060, 390}, "03 / Range-Doppler", "Velocity / m/s; positive = receding | fixed scale -70 to 0 dB power");
        ui::heatmap(a, cfg, map);
        for (const auto& det : detections)
            DrawCircleLines(static_cast<int>(a.x + a.width * (det.range_bin + 0.5) / cfg.range_bins()),
                static_cast<int>(a.y + a.height * (cfg.chirps - det.doppler_bin - 0.5) / cfg.chirps), 5, WHITE);
        EndDrawing();
    }
    CloseWindow();
}
