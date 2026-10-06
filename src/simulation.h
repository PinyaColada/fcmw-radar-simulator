#pragma once
#include "config.h"
#include <random>

namespace radar {
struct Target { double range_m, velocity_mps, amplitude; };
// Complex envelope of the transmitted chirp; RF carrier is represented analytically.
std::vector<Complex> transmit_chirp(const Config& cfg);
class Simulator {
public:
    explicit Simulator(unsigned seed = 42) : rng_(seed) {}
    Cube beat(const Config& cfg, const std::vector<Target>& targets);
private:
    std::mt19937 rng_;
};
}
