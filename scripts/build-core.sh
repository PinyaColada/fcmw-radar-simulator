#!/usr/bin/env bash
# Minimal fallback for machines without CMake. UI still needs the CMake build.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-local
core=(src/config.cpp src/simulation.cpp src/fft.cpp src/range_fft.cpp
      src/doppler_fft.cpp src/cfar.cpp src/tracker.cpp src/demo.cpp src/options.cpp)
flags=(-std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc)
"${CXX:-g++}" "${flags[@]}" "${core[@]}" tests/tests.cpp -o build-local/radar_tests
"${CXX:-g++}" "${flags[@]}" "${core[@]}" src/headless.cpp -o build-local/radar_headless
./build-local/radar_tests
