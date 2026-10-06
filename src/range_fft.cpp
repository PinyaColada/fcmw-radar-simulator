#include "range_fft.h"
#include "fft.h"
#include <stdexcept>

namespace radar {
Cube range_fft(const Config& cfg, const Cube& beat) {
    cfg.validate();
    if (beat.size() != static_cast<std::size_t>(cfg.chirps))
        throw std::invalid_argument("wrong chirp count");
    Cube out;
    for (auto row : beat) {
        if (row.size() != static_cast<std::size_t>(cfg.samples()))
            throw std::invalid_argument("wrong sample count");
        double gain = 0;
        for (int n = 0; n < cfg.samples(); ++n) {
            const double w = cfg.window ? hann(n, cfg.samples()) : 1.0;
            row[n] *= w;
            gain += w;
        }
        fft(row);
        // Normalize coherent window gain: on-bin unit tone has magnitude 1.
        row.resize(cfg.range_bins()); // Use positive half of the signed I/Q band.
        for (auto& value : row) value /= gain;
        out.push_back(std::move(row));
    }
    return out;
}
}
