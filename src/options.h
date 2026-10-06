#pragma once
#include "demo.h"
#include <string>

namespace radar {
struct Options {
    Config radar;
    CfarConfig cfar;
    int targets = 3;
    int frames = 0; // GUI: unlimited; headless: defaults to 200 if zero.
    unsigned seed = 42;
    std::vector<Target> custom_targets;
    std::string output = "radar-frame.svg";
    std::string screenshot;
    bool help = false;
    std::vector<Target> scene() const;
};
Options parse_options(int argc, char** argv);
const char* usage();
}
