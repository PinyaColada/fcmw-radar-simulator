#include "simulation.h"
#include "range_fft.h"
#include "plots.h"
#include <cmath>

int main() {
    radar::Config cfg;
    radar::Simulator sim;
    InitWindow(1100, 650, "FMCW | Range processing");
    SetTargetFPS(30);
    while (!WindowShouldClose()) {
        const auto beat = sim.beat(cfg, {{25, 0, 1}});
        const auto range = radar::range_fft(cfg, beat);
        std::vector<double> raw, spectrum;
        for (auto v : beat[0]) raw.push_back(v.real());
        for (auto v : range[0]) spectrum.push_back(20 * std::log10(std::max(1e-6, std::abs(v))));
        BeginDrawing();
        ClearBackground(ui::background);
        DrawText("FMCW RADAR / range processing", 24, 22, 26, ui::ink);
        ui::slider({30, 70, 360, 50}, "SNR", cfg.snr_db, -30, 40, "%.1f dB");
        auto a = ui::chart({20, 150, 520, 450}, "01 / Beat signal", "First chirp - I component");
        ui::line(a, raw, -4, 4, ui::cyan);
        ui::axes(a, cfg.chirp_s * 1e6, "us", -4, 4);
        a = ui::chart({560, 150, 520, 450}, "02 / Range spectrum", "Hann + radix-2 FFT - dB amplitude");
        ui::line(a, spectrum, -80, 5, ui::gold);
        ui::axes(a, (cfg.range_bins() - 1) * cfg.range_resolution(), "m", -80, 5);
        EndDrawing();
    }
    CloseWindow();
}
