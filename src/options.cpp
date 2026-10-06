#include "options.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace radar {
namespace {
double number(const std::string& text) {
    std::size_t used = 0;
    const double value = std::stod(text, &used);
    if (used != text.size() || !std::isfinite(value)) throw std::invalid_argument("invalid number: " + text);
    return value;
}
int integer(const std::string& text) {
    const double value = number(text);
    if (value < 0 || value > 1000000 || value != std::floor(value))
        throw std::invalid_argument("expected integer in 0..1000000: " + text);
    return static_cast<int>(value);
}
}
std::vector<Target> Options::scene() const {
    return custom_targets.empty() ? default_targets(radar, targets) : custom_targets;
}
Options parse_options(int argc, char** argv) {
    Options out;
    bool count_given = false;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") { out.help = true; return out; }
        if (key == "--no-window") { out.radar.window = false; continue; }
        if (i + 1 >= argc) throw std::invalid_argument("missing value for " + key);
        const std::string value = argv[++i];
        if (key == "--bandwidth-mhz") out.radar.bandwidth_hz = number(value) * 1e6;
        else if (key == "--chirp-us") out.radar.chirp_s = number(value) * 1e-6;
        else if (key == "--sample-rate-mhz") out.radar.sample_rate_hz = number(value) * 1e6;
        else if (key == "--chirps") out.radar.chirps = integer(value);
        else if (key == "--frame-ms") out.radar.frame_s = number(value) * 1e-3;
        else if (key == "--snr") out.radar.snr_db = number(value);
        else if (key == "--targets") { out.targets = integer(value); count_given = true; }
        else if (key == "--cfar-factor") out.cfar.threshold_factor = number(value);
        else if (key == "--guard-range") out.cfar.guard_range = integer(value);
        else if (key == "--guard-doppler") out.cfar.guard_doppler = integer(value);
        else if (key == "--training-range") out.cfar.training_range = integer(value);
        else if (key == "--training-doppler") out.cfar.training_doppler = integer(value);
        else if (key == "--frames") out.frames = integer(value);
        else if (key == "--seed") out.seed = static_cast<unsigned>(integer(value));
        else if (key == "--output") out.output = value;
        else if (key == "--screenshot") out.screenshot = value;
        else if (key == "--target") {
            std::istringstream parts(value);
            std::string range, velocity, amplitude, extra;
            if (!std::getline(parts, range, ',') || !std::getline(parts, velocity, ',') ||
                !std::getline(parts, amplitude, ',') || std::getline(parts, extra, ',') || value.back() == ',')
                throw std::invalid_argument("--target requires range_m,velocity_mps,amplitude");
            out.custom_targets.push_back({number(range), number(velocity), number(amplitude)});
        } else throw std::invalid_argument("unknown option " + key);
    }
    if (count_given && !out.custom_targets.empty()) throw std::invalid_argument("choose --targets or repeated --target");
    out.radar.validate();
    out.cfar.validate(out.radar.range_bins(), out.radar.chirps);
    if (!out.custom_targets.empty()) out.targets = static_cast<int>(out.custom_targets.size());
    Demo validation(out.radar, out.scene(), out.seed);
    return out;
}
const char* usage() {
    return "FMCW radar demo (C++17)\n"
        "  --bandwidth-mhz B      Sweep bandwidth (200)\n"
        "  --chirp-us T           Chirp duration / repetition interval (50)\n"
        "  --sample-rate-mhz Fs   ADC rate (5.12); Fs*T must be a power of 2\n"
        "  --chirps M             Chirps per frame, power of 2 (64)\n"
        "  --frame-ms T           Frame interval, >= coherent frame duration (50)\n"
        "  --snr DB               Unit-echo SNR before processing (5)\n"
        "  --targets N            1..3 preset moving targets (3)\n"
        "  --target R,V,A         Explicit target; repeat 1..3 times instead of --targets\n"
        "  --cfar-factor A        Linear power threshold multiplier > 1 (12)\n"
        "  --guard-range N        Guard half-width in range (2)\n"
        "  --guard-doppler N      Guard half-width in Doppler (2)\n"
        "  --training-range N     Training thickness outside guard in range (4)\n"
        "  --training-doppler N   Training thickness outside guard in Doppler (4)\n"
        "  --no-window            Disable Hann in both FFT dimensions\n"
        "  --seed N               Reproducible noise seed (42)\n"
        "  --frames N             Finite run; 0 = GUI unlimited / headless 200\n"
        "  --output PATH          Headless SVG file (radar-frame.svg)\n"
        "  --screenshot PATH.png  GUI screenshot on exit (use with --frames)\n"
        "  --help                 Show this help\n";
}
}
