#include "doppler_fft.h"
#include "fft.h"
#include <stdexcept>

namespace radar {
double velocity_at(const Config& cfg, int shifted_bin) {
    return (shifted_bin - cfg.chirps / 2) * cfg.velocity_resolution();
}
double corrected_range(const Config& cfg, int range_bin, int shifted_bin) {
    // Beat contains Doppler too: R = c*(fbeat - fd)/(2*S).
    const double fd = 2.0 * velocity_at(cfg, shifted_bin) / cfg.wavelength();
    return range_bin * cfg.range_resolution() - c * fd / (2.0 * cfg.slope());
}
RangeDoppler doppler_fft(const Config& cfg, const Cube& range) {
    cfg.validate();
    if (range.size() != static_cast<std::size_t>(cfg.chirps))
        throw std::invalid_argument("wrong range FFT chirp count");
    for (const auto& row : range)
        if (row.size() != static_cast<std::size_t>(cfg.range_bins()))
            throw std::invalid_argument("wrong range FFT bin count");
    RangeDoppler out{std::vector<std::vector<double>>(cfg.chirps, std::vector<double>(cfg.range_bins()))};
    std::vector<Complex> column(cfg.chirps);
    // Slow-time DFT resolves fd = 2v/lambda from inter-chirp phase increments.
    // Windowing both dimensions trades sidelobes for wider main lobes.
    for (int r = 0; r < cfg.range_bins(); ++r) {
        double gain = 0;
        for (int m = 0; m < cfg.chirps; ++m) {
            const double w = cfg.window ? hann(m, cfg.chirps) : 1.0;
            column[m] = range[m][r] * w;
            gain += w;
        }
        fft(column);
        for (int d = 0; d < cfg.chirps; ++d)
            out.power[(d + cfg.chirps / 2) % cfg.chirps][r] = std::norm(column[d] / gain);
    }
    // HLS: magnitude-squared doubles word width; preserve small noise powers.
    return out;
}
}
