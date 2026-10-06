#pragma once
#include <complex>
#include <vector>

namespace radar {
constexpr double c = 299792458.0;
constexpr double pi = 3.14159265358979323846;
using Complex = std::complex<double>;
using Cube = std::vector<std::vector<Complex>>; // [chirp][sample or range bin]

struct Config {
    double carrier_hz = 77e9;
    double bandwidth_hz = 200e6;
    double chirp_s = 50e-6;
    double sample_rate_hz = 5.12e6;
    int chirps = 64;
    double frame_s = 0.05;
    double snr_db = 5.0;
    bool window = true;
    int samples() const;
    int range_bins() const { return samples() / 2; }
    double slope() const { return bandwidth_hz / chirp_s; }
    double wavelength() const { return c / carrier_hz; }
    double range_resolution() const { return c / (2.0 * bandwidth_hz); }
    double velocity_resolution() const { return wavelength() / (2.0 * chirps * chirp_s); }
    double max_range() const { return c * sample_rate_hz / (4.0 * slope()); }
    double max_velocity() const { return wavelength() / (4.0 * chirp_s); }
    void validate() const;
};
bool power_of_two(int n);
}
