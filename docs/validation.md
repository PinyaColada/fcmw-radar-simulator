# Validation record

Validated on 2026-10-06 in the supplied Linux workspace.

- `./scripts/build-core.sh`: builds the real core, tests and headless executable
  with C++17, `-O2 -Wall -Wextra -Wpedantic`; tests pass.
- `./build-local/radar_headless --frames 200`: 200 processed frames, three
  CFAR peaks and three confirmed tracks at 9.95 seconds; approximately 4.4 ms
  processing time per frame (plot export excluded).
- `radar-frame.svg` is generated from the final numerical frame. The committed
  `headless-preview.png` is a raster rendering of that SVG, visually inspected.
- CLI checks: five valid scenarios (including custom targets, no Hann window,
  alternate timing and custom CFAR geometry) succeed; 18 malformed options,
  invalid configurations and output errors return nonzero.
- The core tests also pass with AddressSanitizer and UndefinedBehaviorSanitizer
  (`-fsanitize=address,undefined`, `ASAN_OPTIONS=detect_leaks=0`). LeakSanitizer
  itself cannot run under this environment's ptrace supervision; leak checking
  is not claimed.
- GUI source and CMake configuration are provided. They remain unverified by
  a real desktop build: this environment lacks CMake, raylib and several X11/GL
  development headers; terminal access to GitHub fails DNS resolution.

Remaining acceptance checks when dependencies are available:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/radar_demo --frames 60 --screenshot screenshot.png
./build/radar_demo
```

Inspect all four panels and the screenshot. Exercise the SNR, target-count,
CFAR threshold and Hann controls, pause/resume, reset and resizing. Check that
marker coordinates agree with the axes and the true/estimated ranges. Repeat
with the alternate timing example in the README. This record deliberately
leaves those checks pending until the actual application can run.
