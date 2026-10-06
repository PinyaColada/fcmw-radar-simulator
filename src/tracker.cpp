#include "tracker.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace radar {
namespace {
void predict(Track& t, double dt) {
    // x=[range, velocity], F=[[1,dt],[0,1]]. Acceleration sigma = 1 m/s^2.
    // P <- F P F' + Q, Q = sigma_a^2 * [dt^2/2,dt]'*[dt^2/2,dt].
    t.range_m += dt * t.velocity_mps;
    const auto p = t.covariance;
    const double dt2 = dt * dt;
    t.covariance = {{{p[0][0] + dt * (p[0][1] + p[1][0]) + dt2 * p[1][1] + dt2 * dt2 / 4,
                     p[0][1] + dt * p[1][1] + dt2 * dt / 2},
                    {p[1][0] + dt * p[1][1] + dt2 * dt / 2, p[1][1] + dt2}}};
}
void correct(Track& t, int component, double measurement, double variance) {
    // Independent range and Doppler measurements: two scalar Kalman updates.
    // K=P H'/(H P H'+R). Joseph form keeps P positive under roundoff.
    auto p = t.covariance;
    const double innovation = measurement - (component == 0 ? t.range_m : t.velocity_mps);
    const double s = p[component][component] + variance;
    const double k[2] = {p[0][component] / s, p[1][component] / s};
    t.range_m += k[0] * innovation;
    t.velocity_mps += k[1] * innovation;
    double a[2][2] = {{1, 0}, {0, 1}};
    for (int i = 0; i < 2; ++i) a[i][component] -= k[i];
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j) {
            double value = k[i] * variance * k[j];
            for (int u = 0; u < 2; ++u)
                for (int v = 0; v < 2; ++v) value += a[i][u] * p[u][v] * a[j][v];
            t.covariance[i][j] = value;
        }
    // HLS: division, covariance dynamic range and symmetry need special care.
}
}
void Tracker::update(const std::vector<Detection>& detections, double dt, double rv, double vv) {
    if (!std::isfinite(dt) || dt <= 0 || !std::isfinite(rv) || rv <= 0 || !std::isfinite(vv) || vv <= 0)
        throw std::invalid_argument("tracker needs positive dt and measurement variances");
    for (const auto& d : detections)
        if (!std::isfinite(d.range_m) || !std::isfinite(d.velocity_mps) || !std::isfinite(d.power))
            throw std::invalid_argument("non-finite tracker measurement");
    for (auto& track : tracks_) predict(track, dt);
    struct Pair { double distance; std::size_t track, detection; };
    std::vector<Pair> pairs;
    for (std::size_t t = 0; t < tracks_.size(); ++t)
        for (std::size_t d = 0; d < detections.size(); ++d) {
            const double dr = detections[d].range_m - tracks_[t].range_m;
            const double dv = detections[d].velocity_mps - tracks_[t].velocity_mps;
            const double sr = tracks_[t].covariance[0][0] + rv;
            const double sv = tracks_[t].covariance[1][1] + vv;
            const double distance = dr * dr / sr + dv * dv / sv;
            if (distance < 16) pairs.push_back({distance, t, d}); // Four-sigma diagonal gate.
        }
    std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
        return std::tie(a.distance, a.track, a.detection) < std::tie(b.distance, b.track, b.detection);
    });
    std::vector<bool> used_track(tracks_.size()), used_detection(detections.size());
    // Greedy nearest neighbour, one detection per track and one track per detection.
    for (const auto& pair : pairs) {
        if (used_track[pair.track] || used_detection[pair.detection]) continue;
        auto& track = tracks_[pair.track];
        const auto& detection = detections[pair.detection];
        correct(track, 0, detection.range_m, rv);
        correct(track, 1, detection.velocity_mps, vv);
        ++track.hits;
        track.misses = 0;
        used_track[pair.track] = used_detection[pair.detection] = true;
    }
    for (std::size_t t = 0; t < tracks_.size(); ++t)
        if (!used_track[t]) ++tracks_[t].misses;
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(), [](const auto& t) {
        return t.misses > (t.confirmed() ? 5 : 1);
    }), tracks_.end());
    // Prefer strong detections for births; bound clutter load independently of truth count.
    std::vector<std::size_t> births;
    for (std::size_t d = 0; d < detections.size(); ++d) if (!used_detection[d]) births.push_back(d);
    std::stable_sort(births.begin(), births.end(), [&](auto a, auto b) { return detections[a].power > detections[b].power; });
    for (auto d : births) {
        if (tracks_.size() >= 24) break;
        tracks_.push_back({next_id_++, detections[d].range_m, detections[d].velocity_mps, {{{rv, 0}, {0, vv}}}});
    }
}
}
