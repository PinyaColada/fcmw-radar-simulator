#pragma once
#include "cfar.h"
#include <array>

namespace radar {
struct Track {
    int id;
    double range_m, velocity_mps;
    std::array<std::array<double, 2>, 2> covariance;
    int hits = 1, misses = 0;
    bool confirmed() const { return hits >= 3; }
};
class Tracker {
public:
    void update(const std::vector<Detection>& detections, double dt, double range_variance, double velocity_variance);
    const std::vector<Track>& tracks() const { return tracks_; }
    void reset() { tracks_.clear(); next_id_ = 1; }
private:
    std::vector<Track> tracks_;
    int next_id_ = 1;
};
}
