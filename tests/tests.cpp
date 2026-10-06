#include "simulation.h"
#include "range_fft.h"
#include "fft.h"
#include "doppler_fft.h"
#include "cfar.h"
#include "tracker.h"
#include "options.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    try {
        std::vector<radar::Complex> impulse(8);
        impulse[0] = 1;
        radar::fft(impulse);
        for (auto v : impulse) require(std::abs(v - radar::Complex(1, 0)) < 1e-12, "FFT impulse");
        radar::Config cfg;
        cfg.snr_db = 120;
        radar::Simulator sim;
        const auto tx = radar::transmit_chirp(cfg);
        for (auto v : tx) require(std::abs(std::abs(v) - 1) < 1e-12, "chirp envelope");
        for (bool window : {false, true}) {
            cfg.window = window;
            const auto beat = sim.beat(cfg, {{32 * cfg.range_resolution(), 0, 1}});
            const auto range = radar::range_fft(cfg, beat);
            const auto peak = std::max_element(range[0].begin(), range[0].end(),
                [](auto a, auto b) { return std::norm(a) < std::norm(b); });
            require(peak - range[0].begin() == 32, "single target expected range bin");
            require(std::abs(std::abs(*peak) - 1) < 1e-5, "coherent FFT gain");
        }
        std::cout << "PASS: FFT impulse, TX envelope, range bin and Hann gain\n";
        for (int velocity_bin : {-12, 0, 9}) {
            const auto beat = sim.beat(cfg, {{40 * cfg.range_resolution(), velocity_bin * cfg.velocity_resolution(), 1}});
            const auto map = radar::doppler_fft(cfg, radar::range_fft(cfg, beat));
            int best_d = 0, best_r = 0;
            for (int d = 0; d < cfg.chirps; ++d)
                for (int r = 0; r < cfg.range_bins(); ++r)
                    if (map.power[d][r] > map.power[best_d][best_r]) { best_d = d; best_r = r; }
            require(best_d == cfg.chirps / 2 + velocity_bin, "signed Doppler bin");
            require(best_r == 40, "range bin with fast-time Doppler");
            require(std::abs(radar::corrected_range(cfg, best_r, best_d) - 40 * cfg.range_resolution()) < cfg.range_resolution() / 2, "range Doppler correction");
        }
        std::cout << "PASS: signed Doppler peaks and range coupling\n";
        radar::CfarConfig cp;
        radar::RangeDoppler flat{std::vector<std::vector<double>>(cfg.chirps, std::vector<double>(cfg.range_bins(), 1))};
        require(radar::cfar(cfg, flat, cp).empty(), "flat noise must not detect");
        flat.power[0][40] = 20;
        flat.power[cfg.chirps - 1][40] = 15;
        auto detections = radar::cfar(cfg, flat, cp);
        require(detections.size() == 1 && detections[0].doppler_bin == 0, "Doppler wrap and local maximum");
        require(std::abs(detections[0].noise_power - 1) < 1e-12, "guard excludes neighbouring peak");
        cp.threshold_factor = 21;
        require(radar::cfar(cfg, flat, cp).empty(), "threshold factor controls detections");
        cp.threshold_factor = 12;
        flat.power[0][40] = flat.power[cfg.chirps - 1][40] = 1;
        flat.power[10][0] = 100;
        require(radar::cfar(cfg, flat, cp).empty(), "range boundary policy");
        const auto single = radar::doppler_fft(cfg, radar::range_fft(cfg,
            sim.beat(cfg, {{32 * cfg.range_resolution(), -4 * cfg.velocity_resolution(), 1}})));
        detections = radar::cfar(cfg, single, cp);
        require(std::any_of(detections.begin(), detections.end(), [&](const auto& d) {
            return d.range_bin == 32 && d.doppler_bin == cfg.chirps / 2 - 4;
        }), "end-to-end single target CFAR");
        std::cout << "PASS: CA-CFAR threshold, guards, boundaries, wrap and target detection\n";
        radar::Tracker tracker;
        std::mt19937 rng(11);
        std::normal_distribution<double> nr(0, 0.3), nv(0, 0.2);
        double raw_error = 0, filtered_error = 0;
        for (int frame = 0; frame < 120; ++frame) {
            const double truth = 20 + 2 * frame * 0.05;
            const double measured = truth + nr(rng);
            tracker.update({{0, 0, measured, 2 + nv(rng), 1, 0.01}}, 0.05, 0.09, 0.04);
            require(tracker.tracks().size() == 1, "stable single track association");
            const auto& t = tracker.tracks()[0];
            const auto& p = t.covariance;
            require(p[0][0] > 0 && p[1][1] > 0 && p[0][0] * p[1][1] - p[0][1] * p[1][0] > 0, "positive covariance");
            require(std::abs(p[0][1] - p[1][0]) < 1e-12, "symmetric covariance");
            if (frame > 10) {
                raw_error += std::pow(measured - truth, 2);
                filtered_error += std::pow(t.range_m - truth, 2);
            }
        }
        require(filtered_error < raw_error * 0.4, "Kalman reduces measurement error");
        require(tracker.tracks()[0].confirmed(), "track confirmation");
        const double before = tracker.tracks()[0].range_m;
        tracker.update({}, 0.05, 0.09, 0.04);
        require(tracker.tracks()[0].range_m > before && tracker.tracks()[0].misses == 1, "coast missed detection");
        for (int i = 0; i < 5; ++i) tracker.update({}, 0.05, 0.09, 0.04);
        require(tracker.tracks().empty(), "delete stale tracks");
        tracker.reset();
        for (int f = 0; f < 40; ++f) {
            // Two targets cross in range; Doppler separates their association.
            std::vector<radar::Detection> ds{{0, 0, 20 + 0.1 * f, 2, 1, 0}, {0, 0, 24 - 0.1 * f, -2, 1, 0}};
            if (f % 2) std::reverse(ds.begin(), ds.end());
            tracker.update(ds, 0.05, 0.09, 0.04);
            require(tracker.tracks().size() == 2, "one-to-one association");
            require(tracker.tracks()[0].id == 1 && tracker.tracks()[0].velocity_mps > 0, "identity through crossing");
        }
        std::cout << "PASS: Kalman error reduction, covariance, coasting, deletion and crossing association\n";
        // Against a direct DFT, including phases: catches FFT permutation/sign bugs.
        std::vector<radar::Complex> input(16);
        for (std::size_t n = 0; n < input.size(); ++n) input[n] = {nr(rng), nv(rng)};
        auto transformed = input;
        radar::fft(transformed);
        for (std::size_t k = 0; k < input.size(); ++k) {
            radar::Complex expected{};
            for (std::size_t n = 0; n < input.size(); ++n)
                expected += input[n] * std::polar(1.0, -2 * radar::pi * k * n / input.size());
            require(std::abs(expected - transformed[k]) < 1e-11, "FFT agrees with direct DFT");
        }
        cfg.snr_db = 10;
        radar::Simulator noise_sim(123);
        const auto noise_only = noise_sim.beat(cfg, {});
        double noise_power = 0;
        radar::Complex mean{};
        for (const auto& row : noise_only) for (auto value : row) { noise_power += std::norm(value); mean += value; }
        noise_power /= cfg.samples() * cfg.chirps;
        mean /= cfg.samples() * cfg.chirps;
        require(std::abs(noise_power - 0.1) < 0.004 && std::abs(mean) < 0.01, "complex Gaussian SNR definition");
        cfg.snr_db = 5;
        radar::CfarConfig multi_cfar;
        radar::Demo demo(cfg, radar::default_targets(cfg, 3), 42);
        int good_frames = 0;
        for (int f = 0; f < 200; ++f) {
            demo.step(cfg, multi_cfar);
            if (f < 10) continue;
            bool all_found = true;
            for (const auto& truth : demo.frame().truth) {
                const auto found = std::find_if(demo.tracks().begin(), demo.tracks().end(), [&](const auto& t) {
                    return t.confirmed() && std::abs(t.range_m - truth.range_m) < cfg.range_resolution() &&
                        std::abs(t.velocity_mps - truth.velocity_mps) < cfg.velocity_resolution();
                });
                if (found == demo.tracks().end()) all_found = false;
            }
            if (all_found) ++good_frames;
        }
        require(good_frames >= 185, "three-target end-to-end tracking coverage");
        require(demo.tracks().size() == 3, "no spurious default-scenario tracks");
        // Change radar timing and FFT sizes, not just default dimensions.
        cfg.bandwidth_hz = 100e6; cfg.chirp_s = 100e-6; cfg.sample_rate_hz = 5.12e6; cfg.chirps = 32;
        cfg.snr_db = 100;
        const auto changed = radar::cfar(cfg, radar::doppler_fft(cfg, radar::range_fft(cfg,
            sim.beat(cfg, {{60 * cfg.range_resolution(), 3 * cfg.velocity_resolution(), 1}}))), multi_cfar);
        require(std::any_of(changed.begin(), changed.end(), [&](const auto& d) {
            return d.range_bin == 60 && d.doppler_bin == cfg.chirps / 2 + 3;
        }), "configurable timing and FFT sizes");
        int rejected = 0;
        try { auto bad = cfg; bad.chirps = 30; bad.validate(); } catch (const std::invalid_argument&) { ++rejected; }
        try { auto bad = cfg; bad.sample_rate_hz = 5.123456e6; bad.validate(); } catch (const std::invalid_argument&) { ++rejected; }
        try { auto bad = multi_cfar; bad.guard_doppler = 999; bad.validate(128, 64); } catch (const std::invalid_argument&) { ++rejected; }
        try { std::vector<radar::Complex> bad(3); radar::fft(bad); } catch (const std::invalid_argument&) { ++rejected; }
        require(rejected == 4, "invalid inputs rejected");
        std::cout << "PASS: DFT reference, Gaussian noise power, 200-frame three-target chain, alternate timing and invalid inputs\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
