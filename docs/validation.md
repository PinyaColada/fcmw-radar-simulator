# Validation record

Validated on 2026-10-06 in the supplied Linux workspace, after the graphics
build dependencies became available. CMake 3.28.3, GCC 13.3.0, raylib 5.5,
GLFW/X11 and Mesa OpenGL 4.6 on AMD integrated graphics.

## Build and numerical tests

The following commands completed successfully:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 6
ctest --test-dir build --output-on-failure
cmake -S . -B build-core -DRADAR_BUILD_UI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core -j 6
ctest --test-dir build-core --output-on-failure
```

Both builds pass `radar_tests` and `headless_smoke` (2/2). FetchContent fetched
the real raylib dependency; the desktop executable linked and created its
OpenGL window successfully. Compiler warnings originate in bundled raylib
code; the project's C++ sources compile without warnings.

- `./scripts/build-core.sh` also builds and passes the same numerical tests
  without CMake, using C++17 and `-O2 -Wall -Wextra -Wpedantic`.
- The 200-frame, three-target headless run produced three CFAR peaks and three
  confirmed tracks at 9.95 seconds, at approximately 4.4 ms processing time per
  frame, excluding plot export.
- Five valid CLI scenarios succeed, including custom targets, no Hann window,
  alternate timing and custom CFAR geometry. Eighteen malformed options,
  invalid configurations and output errors return nonzero.
- Core tests also passed with AddressSanitizer and UndefinedBehaviorSanitizer
  (`-fsanitize=address,undefined`, `ASAN_OPTIONS=detect_leaks=0`). Leak checking
  was disabled because the initial restricted run used ptrace supervision.

## Actual desktop checks

`./build/radar_demo --frames 60 --screenshot docs/screenshot.png` completed
successfully. The committed screenshot comes from the desktop renderer and
was visually inspected; it shows three confirmed tracks, all four panels and
60 FPS. The separate `headless-preview.png` comes from the SVG exporter.

The live application was exercised through X11 input and captured:

- SNR changed from 5 dB to approximately -25 dB: raw I/Q noise and both spectral
  noise floors rose visibly.
- Target count changed from three to two: the scene reset and showed two truth
  positions.
- CFAR threshold changed from 12x to approximately 33.6x; the noisy scenario
  showed no accepted peaks in the captured frame.
- Hann toggled off; the later high-SNR capture showed the expected stronger
  sidelobes in the range and Doppler directions.
- Pause froze the plot and timestamp. Two window captures one second apart
  had zero differing pixels when the FPS footer was excluded.
- Resume advanced time. Reset while paused returned the displayed time to
  0.00 s, with tracks awaiting confirmation again. Keyboard events were held
  for 150 ms so the polling UI could observe each press.
- Resizing from 1440x960 to the 1200x850 minimum retained all panels and controls.
  Subtitle sizes adapt to panel width. Both sizes were visually inspected.
- Escape closed the application cleanly.

An alternate desktop run also completed and was visually inspected:

```sh
./build/radar_demo --bandwidth-mhz 150 --chirp-us 100 \
  --sample-rate-mhz 2.56 --chirps 128 --frames 30 \
  --screenshot build/gui-alternate.png
```

Its axes updated to approximately 1 m range bins, 0.152 m/s velocity bins,
127.9 m maximum range and +/-9.7 m/s speed, with three confirmed tracks.

Screenshot export preserves the requested directory (raylib's convenience
`TakeScreenshot` strips it, so the application uses `ExportImage` directly).
A nonexistent output directory was checked: the application closed cleanly
and returned exit status 1 with a useful error.

Local integration logs and supplementary captures are in ignored `build/`.
The committed desktop capture is `docs/screenshot.png`.
