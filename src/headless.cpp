#include "options.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
// Small offline plot exporter: same numerical frame as the GUI, no graphics dependency.
void snapshot(const std::string& path, const radar::Config& cfg, const radar::Demo& demo) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot open SVG output: " + path);
    out << std::fixed << std::setprecision(2);
    out << "<svg xmlns='http://www.w3.org/2000/svg' width='1280' height='860' viewBox='0 0 1280 860'>"
           "<rect width='1280' height='860' fill='#0c131e'/>";
    const auto text = [&](double x, double y, const std::string& message, const char* color = "#dde6f0", int size = 15) {
        out << "<text x='" << x << "' y='" << y << "' fill='" << color << "' font-family='sans-serif' font-size='" << size << "'>" << message << "</text>";
    };
    const auto number = [](double value, int precision = 1) {
        std::ostringstream s; s << std::fixed << std::setprecision(precision) << value; return s.str();
    };
    const auto& frame = demo.frame();
    text(30, 36, "FMCW / SIGNAL TO TRACK", "#dde6f0", 25);
    text(30, 62, "Offline frame from the processing core | t = " + number(frame.time_s, 2) + " s | SNR " + number(cfg.snr_db) + " dB | Hann " + (cfg.window ? "ON" : "OFF"), "#8fa3b9");
    const auto panel = [&](double x, double y, const char* title, const char* subtitle) {
        out << "<rect x='" << x << "' y='" << y << "' width='605' height='350' rx='10' fill='#141f2e'/>";
        text(x + 18, y + 29, title, "#dde6f0", 20);
        text(x + 18, y + 53, subtitle, "#8fa3b9", 13);
    };
    panel(20, 85, "01 / Raw beat signal", "First chirp | I: cyan, Q: amber | amplitude");
    panel(645, 85, "02 / Range spectrum", "First chirp | coherent-gain-normalized amplitude / dB");
    panel(20, 455, "03 / Range-Doppler + CFAR", "Velocity / m/s | positive = receding | white circles = detections");
    panel(645, 455, "04 / Top-down tracks", "Boresight only, x = 0 | single receiver: no angle estimate");
    constexpr double width = 500, height = 225;
    const auto axes = [&](double x, double y, double xmax, double ymin, double ymax, const char* unit, double xmin = 0) {
        for (int i = 0; i <= 4; ++i) {
            const double xx = x + width * i / 4, yy = y + height * i / 4;
            out << "<path d='M " << xx << ' ' << y << " v " << height << " M " << x << ' ' << yy << " h " << width << "' stroke='#283646' fill='none'/>";
            text(xx - 10, y + height + 21, number(xmin + (xmax - xmin) * i / 4), "#8fa3b9", 12);
        }
        text(x - 48, y + 5, number(ymax), "#8fa3b9", 12);
        text(x - 48, y + height, number(ymin), "#8fa3b9", 12);
        text(x + width / 2 - 20, y + height + 38, unit, "#8fa3b9", 12);
    };
    double raw_max = 1;
    for (auto v : frame.beat[0]) raw_max = std::max({raw_max, std::abs(v.real()), std::abs(v.imag())});
    axes(80, 165, (cfg.samples() - 1) / cfg.sample_rate_hz * 1e6, -raw_max, raw_max, "time / us");
    axes(705, 165, (cfg.range_bins() - 1) * cfg.range_resolution(), -80, 20, "range / m");
    for (int plot = 0; plot < 3; ++plot) {
        const int count = plot == 2 ? cfg.range_bins() : cfg.samples();
        out << "<polyline fill='none' stroke='" << (plot == 1 ? "#ffc55d" : "#3ed3d8") << "' stroke-width='1.2' points='";
        for (int n = 0; n < count; ++n) {
            const double value = plot == 2 ? (20 * std::log10(std::max(1e-6, std::abs(frame.range[0][n]))) + 80) / 100
                : ((plot == 0 ? frame.beat[0][n].real() : frame.beat[0][n].imag()) + raw_max) / (2 * raw_max);
            out << (plot == 2 ? 705 : 80) + width * n / (count - 1) << ',' << 165 + height * (1 - std::clamp(value, 0.0, 1.0)) << ' ';
        }
        out << "'/>";
    }
    // Cell centres match the GUI's Doppler/range convention, fixed power scale.
    for (int d = 0; d < cfg.chirps; ++d)
        for (int r = 0; r < cfg.range_bins(); ++r) {
            const double db = 10 * std::log10(std::max(1e-12, frame.map.power[d][r]));
            const double t = std::clamp((db + 70) / 70, 0.0, 1.0);
            const int stops[4][3] = {{13, 22, 43}, {32, 63, 127}, {19, 156, 173}, {247, 215, 96}};
            const int k = std::min(2, static_cast<int>(t * 3));
            const double f = t * 3 - k;
            out << "<rect x='" << 80 + width * r / cfg.range_bins() << "' y='" << 535 + height * (cfg.chirps - 1 - d) / cfg.chirps
                << "' width='" << width / cfg.range_bins() + 0.1 << "' height='" << height / cfg.chirps + 0.1 << "' fill='rgb(";
            for (int channel = 0; channel < 3; ++channel) {
                if (channel) out << ',';
                out << static_cast<int>(stops[k][channel] * (1 - f) + stops[k + 1][channel] * f);
            }
            out << ")'/>";
        }
    axes(80, 535, cfg.max_range() - cfg.range_resolution() / 2,
        -cfg.max_velocity() - cfg.velocity_resolution() / 2,
        cfg.max_velocity() - cfg.velocity_resolution() / 2, "range / m", -cfg.range_resolution() / 2);
    for (const auto& d : frame.detections)
        out << "<circle cx='" << 80 + width * (d.range_bin + 0.5) / cfg.range_bins() << "' cy='"
            << 535 + height * (cfg.chirps - d.doppler_bin - 0.5) / cfg.chirps << "' r='5' fill='none' stroke='white'/>";
    for (int i = 0; i <= 4; ++i) {
        const double y = 760 - height * i / 4;
        out << "<path d='M 705 " << y << " h 500' stroke='#283646'/>";
        text(657, y + 4, number(cfg.max_range() * i / 4, 0) + " m", "#8fa3b9", 12);
    }
    out << "<path d='M 905 535 V 760' stroke='#8fa3b9'/>";
    const auto y = [&](double range) { return 760 - height * range / cfg.max_range(); };
    for (const auto& t : frame.truth)
        out << "<circle cx='905' cy='" << y(t.range_m) << "' r='9' fill='none' stroke='#3ed3d8' stroke-width='2'/>";
    for (const auto& d : frame.detections)
        out << "<path d='M 900 " << y(d.range_m) - 5 << " l 10 10 m 0 -10 l -10 10' stroke='#ffc55d' stroke-width='2'/>";
    for (const auto& t : demo.tracks()) {
        if (!t.confirmed()) continue;
        out << "<rect x='899' y='" << y(t.range_m) - 6 << "' width='12' height='12' fill='none' stroke='" << (t.misses ? "#8fa3b9" : "#00e430") << "' stroke-width='2'/>";
        text(922, y(t.range_m) + 4, "#" + std::to_string(t.id) + "  " + number(t.range_m) + " m / " + number(t.velocity_mps) + " m/s", "#dde6f0", 13);
    }
    text(722, 786, "O truth (cyan) / X detection (amber) / square Kalman (green)", "#8fa3b9", 12);
    text(30, 837, "CFAR peaks: " + std::to_string(frame.detections.size()) + " | Heatmap fixed scale: -70 to 0 dB power | Not a desktop screenshot", "#8fa3b9", 14);
    out << "</svg>\n";
    out.close();
    if (!out) throw std::runtime_error("failed writing SVG output: " + path);
}
}

int main(int argc, char** argv) {
    try {
        const auto options = radar::parse_options(argc, argv);
        if (options.help) { std::cout << radar::usage(); return 0; }
        if (!options.screenshot.empty()) throw std::invalid_argument("use --output for headless SVG; --screenshot is GUI-only");
        radar::Demo demo(options.radar, options.scene(), options.seed);
        const int frames = options.frames ? options.frames : 200;
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < frames; ++i) demo.step(options.radar, options.cfar);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / frames;
        snapshot(options.output, options.radar, demo);
        std::cout << "Processed " << frames << " frames, " << ms << " ms/frame; t=" << demo.frame().time_s << " s\n";
        std::cout << "CFAR peaks: " << demo.frame().detections.size() << '\n';
        for (const auto& t : demo.tracks())
            if (t.confirmed()) std::cout << "Track #" << t.id << ": range=" << t.range_m << " m, velocity=" << t.velocity_mps << " m/s, misses=" << t.misses << '\n';
        std::cout << "Plot: " << options.output << '\n';
    } catch (const std::exception& e) { std::cerr << "radar_headless: " << e.what() << '\n'; return 1; }
}
