#include "config.h"
#include <cmath>
#include <stdexcept>

namespace radar {
bool power_of_two(int n) { return n > 0 && (n & (n - 1)) == 0; }
int Config::samples() const {
    const double n = sample_rate_hz * chirp_s;
    if (!std::isfinite(n) || n < 32 || n > 4096 || std::abs(n - std::round(n)) > 1e-6)
        throw std::invalid_argument("sample_rate * chirp_duration must be an integer in [32, 4096]");
    return static_cast<int>(std::round(n));
}
void Config::validate() const {
    if (!std::isfinite(carrier_hz) || carrier_hz <= 0 ||
        !std::isfinite(bandwidth_hz) || bandwidth_hz <= 0 || bandwidth_hz >= carrier_hz ||
        !std::isfinite(chirp_s) || chirp_s <= 0 ||
        !std::isfinite(sample_rate_hz) || sample_rate_hz <= 0 ||
        !std::isfinite(frame_s) || frame_s < chirps * chirp_s ||
        !std::isfinite(snr_db) || snr_db < -100 || snr_db > 150)
        throw std::invalid_argument("invalid radar timing, frequencies or SNR");
    if (!power_of_two(samples()) || !power_of_two(chirps) || chirps < 16 || chirps > 1024)
        throw std::invalid_argument("FFT sizes must be powers of two: samples 32..4096, chirps 16..1024");
}
}
