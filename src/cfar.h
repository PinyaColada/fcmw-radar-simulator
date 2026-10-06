#pragma once
#include "doppler_fft.h"
namespace radar {
struct CfarConfig {
    int training_range = 4, training_doppler = 4;
    int guard_range = 2, guard_doppler = 2;
    double threshold_factor = 12.0; // Multiplier of mean LINEAR noise power.
    void validate(int ranges, int dopplers) const;
};
struct Detection {
    int range_bin, doppler_bin;
    double range_m, velocity_mps, power, noise_power;
};
std::vector<Detection> cfar(const Config& cfg, const RangeDoppler& map, const CfarConfig& params);
}
