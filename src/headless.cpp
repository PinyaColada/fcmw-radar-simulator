#include "simulation.h"
#include "range_fft.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

int main() {
    radar::Config cfg;
    radar::Simulator sim;
    const auto beat = sim.beat(cfg, {{25, 0, 1}});
    const auto range = radar::range_fft(cfg, beat);
    std::ofstream svg("radar-frame.svg");
    svg << "<svg xmlns='http://www.w3.org/2000/svg' width='1000' height='460' viewBox='0 0 1000 460'>"
           "<rect width='1000' height='460' fill='#0c131e'/>"
           "<g fill='#dde6f0' font-family='sans-serif'><text x='40' y='35' font-size='24'>FMCW / simulation + range FFT</text>"
           "<text x='40' y='75'>Beat signal (I) / sample index</text><text x='530' y='75'>Range spectrum / metres / -80 to +5 dB</text></g>";
    for (int plot = 0; plot < 2; ++plot) {
        const int count = plot ? cfg.range_bins() : cfg.samples();
        svg << "<polyline fill='none' stroke='" << (plot ? "#ffc55d" : "#3ed3d8") << "' points='";
        for (int n = 0; n < count; ++n) {
            const double v = plot ? (20 * std::log10(std::max(1e-6, std::abs(range[0][n]))) + 80) / 85
                                  : (beat[0][n].real() + 4) / 8;
            svg << 40 + plot * 490 + 430.0 * n / (count - 1) << ',' << 400 - 290 * std::clamp(v, 0.0, 1.0) << ' ';
        }
        svg << "'/>";
    }
    svg << "</svg>\n";
    if (!svg) return 1;
    const auto peak = std::max_element(range[0].begin(), range[0].end(),
        [](auto a, auto b) { return std::norm(a) < std::norm(b); });
    std::cout << "Peak range: " << (peak - range[0].begin()) * cfg.range_resolution() << " m; plot: radar-frame.svg\n";
}
