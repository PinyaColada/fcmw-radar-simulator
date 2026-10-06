#include "simulation.h"
#include <cmath>
#include <stdexcept>

namespace radar {
std::vector<Complex> transmit_chirp(const Config& cfg) {
    cfg.validate();
    std::vector<Complex> tx(cfg.samples());
    for (int n = 0; n < cfg.samples(); ++n) {
        const double t = n / cfg.sample_rate_hz;
        tx[n] = std::polar(1.0, pi * cfg.slope() * t * t);
    }
    return tx;
}

Cube Simulator::beat(const Config& cfg, const std::vector<Target>& targets) {
    cfg.validate();
    if (targets.size() > 3) throw std::invalid_argument("at most three targets");
    for (const auto& target : targets)
        if (!std::isfinite(target.range_m) || target.range_m < 0 ||
            !std::isfinite(target.velocity_mps) ||
            !std::isfinite(target.amplitude) || target.amplitude < 0)
            throw std::invalid_argument("target values must be finite, range/amplitude nonnegative");
    // Unit-amplitude echo power is 1. Each independent I/Q noise component has
    // variance 10^(-SNR/10)/2, independent of the number of active targets.
    const double sigma = std::sqrt(std::pow(10.0, -cfg.snr_db / 10.0) / 2.0);
    std::normal_distribution<double> noise(0.0, sigma);
    Cube out(cfg.chirps, std::vector<Complex>(cfg.samples()));
    for (int m = 0; m < cfg.chirps; ++m) {
        for (int n = 0; n < cfg.samples(); ++n) {
            Complex value(0, 0);
            const double t = n / cfg.sample_rate_hz;
            for (const auto& target : targets) {
                const double tau = 2.0 * target.range_m / c;
                const double fd = 2.0 * target.velocity_mps / cfg.wavelength();
                // Dechirped analytic I/Q signal, positive velocity = receding.
                // Stop-and-hop: R is frozen within this short coherent frame.
                // phi = 2pi[(S*tau+fd)t + fd*m*T + fc*tau - S*tau^2/2].
                const double phase = 2.0 * pi * ((cfg.slope() * tau + fd) * t +
                    fd * m * cfg.chirp_s + cfg.carrier_hz * tau - cfg.slope() * tau * tau / 2.0);
                value += std::polar(target.amplitude, phase);
            }
            const double i = noise(rng_);
            const double q = noise(rng_);
            out[m][n] = value + Complex(i, q);
        }
    }
    // HLS: phase accumulator/LUT, ADC I/Q quantization and saturation belong here.
    return out;
}
}
