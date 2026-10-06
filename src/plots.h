#pragma once
#include "raylib.h"
#include "doppler_fft.h"
#include <string>
#include <vector>
namespace ui {
constexpr Color background{12, 19, 30, 255};
constexpr Color panel{20, 31, 46, 255};
constexpr Color ink{221, 230, 240, 255};
constexpr Color muted{143, 163, 185, 255};
constexpr Color cyan{62, 211, 216, 255};
constexpr Color gold{255, 197, 93, 255};
Rectangle chart(Rectangle outer, const std::string& title, const std::string& subtitle);
void line(Rectangle area, const std::vector<double>& values, double low, double high, Color color);
void axes(Rectangle area, double xmax, const char* unit, double ymin, double ymax);
bool slider(Rectangle area, const char* title, double& value, double low, double high, const char* format);
bool button(Rectangle area, const char* title);
Color heat_color(double db);
void heatmap(Rectangle area, const radar::Config& cfg, const radar::RangeDoppler& map);
}
