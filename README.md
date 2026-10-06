# FMCW radar demo

C++17 radar processing, with a raylib desktop UI and a dependency-free core.

On Ubuntu/Debian:

```sh
sudo apt install build-essential cmake git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/radar_demo
```

CMake FetchContent downloads raylib 5.5 (including GLFW). An X11 desktop or
XWayland and OpenGL 3.3 are required. To build the processing code offline,
use `-DRADAR_BUILD_UI=OFF`. Run `./build/radar_headless` for an SVG plot.

Stage 1: analytic complex beat generation, configurable SNR, Hann window and
range FFT. No RF-rate sampling: the 77 GHz carrier is represented in phase.

Screenshot placeholder: the completed desktop demo will appear here.
