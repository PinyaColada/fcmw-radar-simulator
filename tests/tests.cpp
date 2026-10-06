#include "simulation.h"
#include "range_fft.h"
#include "fft.h"
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
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
