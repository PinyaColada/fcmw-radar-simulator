#pragma once
#include "range_fft.h"
#include "simulation.h"
#include "tracker.h"

namespace radar {
struct Frame {
    double time_s = 0;
    std::vector<Target> truth;
    Cube beat, range;
    RangeDoppler map;
    std::vector<Detection> detections;
};
std::vector<Target> default_targets(const Config& cfg, int count);
// Shared orchestration for GUI and headless demo. Processing modules know
// nothing about UI, truth labels or the demo's reflecting scene.
class Demo {
public:
    Demo(const Config& cfg, const std::vector<Target>& targets, unsigned seed = 42);
    void step(const Config& cfg, const CfarConfig& cfar_cfg);
    const Frame& frame() const { return frame_; }
    const std::vector<Track>& tracks() const { return tracker_.tracks(); }
private:
    Simulator simulator_;
    Tracker tracker_;
    std::vector<Target> targets_;
    Frame frame_;
    double time_s_ = 0;
};
}
