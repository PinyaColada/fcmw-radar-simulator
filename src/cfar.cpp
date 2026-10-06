#include "cfar.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace radar {
void CfarConfig::validate(int ranges, int dopplers) const {
    if (training_range < 1 || training_doppler < 1 || guard_range < 0 || guard_doppler < 0 ||
        training_range > ranges || guard_range > ranges ||
        training_doppler > dopplers || guard_doppler > dopplers ||
        2 * (training_range + guard_range) + 1 > ranges ||
        2 * (training_doppler + guard_doppler) + 1 > dopplers ||
        !std::isfinite(threshold_factor) || threshold_factor <= 1)
        throw std::invalid_argument("CFAR window must fit map; training >= 1, guard >= 0, factor > 1");
}
std::vector<Detection> cfar(const Config& cfg, const RangeDoppler& map, const CfarConfig& p) {
    cfg.validate();
    const int nr = cfg.range_bins(), nd = cfg.chirps;
    p.validate(nr, nd);
    if (map.power.size() != static_cast<std::size_t>(nd)) throw std::invalid_argument("CFAR Doppler size");
    for (const auto& row : map.power) {
        if (row.size() != static_cast<std::size_t>(nr)) throw std::invalid_argument("CFAR range size");
        for (double value : row)
            if (!std::isfinite(value) || value < 0) throw std::invalid_argument("CFAR needs finite linear powers");
    }
    const int er = p.training_range + p.guard_range, ed = p.training_doppler + p.guard_doppler;
    const int count = (2 * er + 1) * (2 * ed + 1) - (2 * p.guard_range + 1) * (2 * p.guard_doppler + 1);
    std::vector<Detection> detections;
    // CA-CFAR: sum the outer rectangle minus the guard rectangle (including CUT).
    // Threshold = alpha * mean(training power). Never average logarithmic dB.
    // Skip range edges; Doppler wraps because its spectrum is periodic.
    for (int d = 0; d < nd; ++d) {
        for (int r = er; r < nr - er; ++r) {
            const double cut = map.power[d][r];
            double sum = 0;
            for (int dd = -ed; dd <= ed; ++dd)
                for (int rr = -er; rr <= er; ++rr)
                    if (std::abs(dd) > p.guard_doppler || std::abs(rr) > p.guard_range)
                        sum += map.power[(d + dd + nd) % nd][r + rr];
            const double noise = sum / count;
            if (cut <= p.threshold_factor * noise) continue;
            // One 3x3 local maximum per lobe; deterministic ties avoid duplicates.
            bool peak = true;
            for (int dd = -1; dd <= 1; ++dd)
                for (int rr = -1; rr <= 1; ++rr) {
                    if (dd == 0 && rr == 0) continue;
                    const int other_d = (d + dd + nd) % nd, other_r = r + rr;
                    const double other = map.power[other_d][other_r];
                    if (other > cut || (other == cut && other_d * nr + other_r < d * nr + r)) peak = false;
                }
            if (peak) detections.push_back({r, d, corrected_range(cfg, r, d), velocity_at(cfg, d), cut, noise});
        }
    }
    // HLS: sum needs ceil(log2(training-cell count)) extra accumulator bits.
    return detections;
}
}
