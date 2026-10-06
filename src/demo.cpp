#include "demo.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace radar {
std::vector<Target> default_targets(const Config& cfg, int count) {
    if (count < 1 || count > 3) throw std::invalid_argument("target count must be 1..3");
    std::vector<Target> targets{{0.26 * cfg.max_range(), 0.15 * cfg.max_velocity(), 1.0},
        {0.53 * cfg.max_range(), -0.10 * cfg.max_velocity(), 0.8},
        {0.74 * cfg.max_range(), 0.04 * cfg.max_velocity(), 0.65}};
    targets.resize(count);
    return targets;
}
Demo::Demo(const Config& cfg, const std::vector<Target>& targets, unsigned seed)
    : simulator_(seed), targets_(targets) {
    cfg.validate();
    if (targets.empty() || targets.size() > 3) throw std::invalid_argument("demo needs 1..3 targets");
    for (const auto& t : targets)
        if (!std::isfinite(t.range_m) || t.range_m < 0.08 * cfg.max_range() || t.range_m > 0.92 * cfg.max_range() ||
            !std::isfinite(t.velocity_mps) || std::abs(t.velocity_mps) >= cfg.max_velocity() ||
            !std::isfinite(t.amplitude) || t.amplitude <= 0)
            throw std::invalid_argument("demo target: range in 8..92% of Rmax, |velocity| < vmax, amplitude > 0");
}
void Demo::step(const Config& cfg, const CfarConfig& cfar_cfg) {
    frame_.time_s = time_s_;
    frame_.truth = targets_;
    frame_.beat = simulator_.beat(cfg, targets_);
    frame_.range = range_fft(cfg, frame_.beat);
    frame_.map = doppler_fft(cfg, frame_.range);
    frame_.detections = cfar(cfg, frame_.map, cfar_cfg);
    const double rv = std::max(0.01, std::pow(cfg.range_resolution(), 2) / 12);
    const double vv = std::max(0.01, std::pow(cfg.velocity_resolution(), 2) / 12);
    tracker_.update(frame_.detections, cfg.frame_s, rv, vv);
    time_s_ += cfg.frame_s;
    // Reflect to keep the demonstration running. These abrupt accelerations
    // violate constant velocity, so the tracker may briefly reacquire.
    const double low = 0.08 * cfg.max_range(), high = 0.92 * cfg.max_range();
    const double width = high - low;
    for (auto& t : targets_) {
        const double unfolded = t.range_m - low + t.velocity_mps * cfg.frame_s;
        double wrapped = std::fmod(unfolded, 2 * width);
        if (wrapped < 0) wrapped += 2 * width;
        if (wrapped > width) { t.range_m = high - (wrapped - width); t.velocity_mps = -t.velocity_mps; }
        else t.range_m = low + wrapped;
    }
}
}
