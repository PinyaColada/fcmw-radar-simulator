#pragma once
#include "config.h"
namespace radar {
struct RangeDoppler {
    // Linear power, [shifted Doppler bin][range bin]. Zero velocity is row M/2.
    std::vector<std::vector<double>> power;
};
RangeDoppler doppler_fft(const Config& cfg, const Cube& range);
double velocity_at(const Config& cfg, int shifted_bin);
double corrected_range(const Config& cfg, int range_bin, int shifted_bin);
}
