#include "simulation.h"
#include "range_fft.h"
#include "fft.h"
#include "doppler_fft.h"
#include "cfar.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        std::vector<radar::Complex> impulse(8);
        impulse[0] = 1;
        radar::fft(impulse);
        for (auto v : impulse) require(std::abs(v - radar::Complex(1, 0)) < 1e-12, "FFT impulse");
        radar::Config cfg;
        cfg.snr_db = 120;
        radar::Simulator sim;
        const auto tx = radar::transmit_chirp(cfg);
        for (auto v : tx) require(std::abs(std::abs(v) - 1) < 1e-12, "chirp envelope");
        for (bool window : {false, true}) {
            cfg.window = window;
            const auto beat = sim.beat(cfg, {{32 * cfg.range_resolution(), 0, 1}});
            const auto range = radar::range_fft(cfg, beat);
            const auto peak = std::max_element(range[0].begin(), range[0].end(),
                [](auto a, auto b) { return std::norm(a) < std::norm(b); });
            require(peak - range[0].begin() == 32, "single target expected range bin");
            require(std::abs(std::abs(*peak) - 1) < 1e-5, "coherent FFT gain");
        }
        std::cout << "PASS: FFT impulse, TX envelope, range bin and Hann gain\n";
        for (int velocity_bin : {-12, 0, 9}) {
            const auto beat = sim.beat(cfg, {{40 * cfg.range_resolution(), velocity_bin * cfg.velocity_resolution(), 1}});
            const auto map = radar::doppler_fft(cfg, radar::range_fft(cfg, beat));
            int best_d = 0, best_r = 0;
            for (int d = 0; d < cfg.chirps; ++d)
                for (int r = 0; r < cfg.range_bins(); ++r)
                    if (map.power[d][r] > map.power[best_d][best_r]) { best_d = d; best_r = r; }
            require(best_d == cfg.chirps / 2 + velocity_bin, "signed Doppler bin");
            require(best_r == 40, "range bin with fast-time Doppler");
            require(std::abs(radar::corrected_range(cfg, best_r, best_d) - 40 * cfg.range_resolution()) < cfg.range_resolution() / 2, "range Doppler correction");
        }
        std::cout << "PASS: signed Doppler peaks and range coupling\n";
        radar::CfarConfig cp;
        radar::RangeDoppler flat{std::vector<std::vector<double>>(cfg.chirps, std::vector<double>(cfg.range_bins(), 1))};
        require(radar::cfar(cfg, flat, cp).empty(), "flat noise must not detect");
        flat.power[0][40] = 20;
        flat.power[cfg.chirps - 1][40] = 15;
        auto detections = radar::cfar(cfg, flat, cp);
        require(detections.size() == 1 && detections[0].doppler_bin == 0, "Doppler wrap and local maximum");
        require(std::abs(detections[0].noise_power - 1) < 1e-12, "guard excludes neighbouring peak");
        cp.threshold_factor = 21;
        require(radar::cfar(cfg, flat, cp).empty(), "threshold factor controls detections");
        cp.threshold_factor = 12;
        flat.power[0][40] = flat.power[cfg.chirps - 1][40] = 1;
        flat.power[10][0] = 100;
        require(radar::cfar(cfg, flat, cp).empty(), "range boundary policy");
        const auto single = radar::doppler_fft(cfg, radar::range_fft(cfg,
            sim.beat(cfg, {{32 * cfg.range_resolution(), -4 * cfg.velocity_resolution(), 1}})));
        detections = radar::cfar(cfg, single, cp);
        require(std::any_of(detections.begin(), detections.end(), [&](const auto& d) {
            return d.range_bin == 32 && d.doppler_bin == cfg.chirps / 2 - 4;
        }), "end-to-end single target CFAR");
        std::cout << "PASS: CA-CFAR threshold, guards, boundaries, wrap and target detection\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
