# ScopeBuddy

ScopeBuddy is an ESP-IDF/LVGL application for the Elecrow CrowPanel Advance
5-inch ESP32-P4 display. It provides a touch interface and rotary-encoder input
for practical oscilloscope measurement exercises. Firmware 0.5 generates
repeatable digital signals and reveals the expected measurements on demand.

## Install from the browser

The easiest installation method does not require ESP-IDF, Python, or Git:

**[Open the ScopeBuddy web installer](https://johannesboernsen.github.io/ScopeBuddy/)**

1. Open the installer in Chrome or Edge on a desktop computer.
2. Connect an Elecrow CrowPanel Advance 5-inch ESP32-P4 with a USB data cable.
3. Select **Connect device**, choose the CrowPanel serial port, and confirm the
   installation.
4. Keep the USB cable connected until the installer displays **Wrapping up**.
   At that point the firmware has been transferred; briefly press **RST** once
   to start ScopeBuddy and finish the installation.

The installer is intended only for the Elecrow CrowPanel Advance 5-inch with
ESP32-P4. A browser installation resets settings stored by an earlier ScopeBuddy
installation; selecting **Erase device** additionally removes all remaining
flash data. Firefox and Safari do not currently expose the Web Serial API
required by the installer.

If the board is not detected automatically, close other programs using its
serial port and try another USB data cable. As a fallback, hold **BOOT**, briefly
press **RESET**, and then release **BOOT** before connecting again.

The CrowPanel does not reliably perform the final automatic reset requested by
ESP Web Tools. This is why pressing **RST** at **Wrapping up** is a normal,
board-specific part of the browser installation and does not indicate a failed
flash operation.

## Lessons

ScopeBuddy 0.5 includes 15 lessons with three difficulty levels each. Nine are
single-channel exercises:

- Periodic signal: frequency, period, and duty cycle
- Pulse widths: HIGH time, LOW time, and duty cycle
- Burst: pulse count, edge-to-edge burst duration, and LOW pause
- Missing pulse: base period and enlarged edge interval
- Servo signal: repetition period, pulse width, and target angle
- Tachometer signal: pulse frequency and rotational speed
- Button bounce: edge count and bounce duration
- UART 8N1: bit time, nominal baud rate, and data byte
- Alternating states: two frequencies and their state duration

Six additional lessons use two synchronized channels:

- Trigger/response: trigger width, response width, and reaction time
- Phase shift: period, edge delay, and phase angle
- Frequency divider: input/output frequency and integer divider ratio
- Ultrasonic echo: response delay, echo width, and calculated distance
- Gated PWM: gate duration, PWM start delay, and pulse count
- Quadrature encoder: period, quarter-period edge offset, and leading track

The continuous PWM lessons use the ESP32-P4 LEDC peripheral. Finite and
irregular pulse sequences use the RMT peripheral at 1 MHz resolution so their
displayed timing values match the generated hardware sequence.

## Hardware

- Elecrow CrowPanel Advance 5-inch ESP32-P4, 800 × 480
- Optional rotary encoder; see [ENCODER.md](ENCODER.md) for wiring
- Oscilloscope CH1 connected to `GPIO48`, CH2 to `GPIO47`, and both grounds to
  board `GND` for two-channel lessons

GPIO47 and GPIO48 are 3.3 V logic outputs intended for high-impedance
oscilloscope or logic-analyzer inputs. Do not connect loads or external
voltages to them.

## Build

Install ESP-IDF 5.4 or newer, activate its environment, and run:

```sh
idf.py set-target esp32p4
idf.py build
idf.py flash monitor
```

ESP-IDF's component manager downloads LVGL, the ESP LVGL port, and the display
and touch-controller drivers declared in `main/idf_component.yml`. The checked-in
`dependencies.lock` pins the resolved component versions used for release builds.

ScopeBuddy 0.5 is built and hardware-tested with ESP-IDF 5.4.2.

## Web installer maintenance

The static installer lives in `docs/`. Its ESP Web Tools manifest points to the
merged firmware image produced by `.github/workflows/pages.yml`. On every push
to `main`, GitHub Actions:

1. builds ScopeBuddy with ESP-IDF 5.4.2;
2. merges the bootloader, partition table, OTA data, and application into one
   browser-installable image;
3. generates a SHA-256 checksum; and
4. deploys the installer and firmware to GitHub Pages.

Before the first deployment, open **Settings → Pages** in the GitHub repository
and choose **GitHub Actions** as the build and deployment source. The finished
site is published at:

```text
https://johannesboernsen.github.io/ScopeBuddy/
```

Keep these three version values in sync for a release:

- `PROJECT_VER` in `CMakeLists.txt`
- `SCOPEBUDDY_FIRMWARE_VERSION` in `main/ui/ui_events.c`
- `version` and the firmware filename in `docs/manifest.json`

The Pages workflow currently publishes version 0.5 from `main`. When changing
the filename in the manifest, change the two matching paths in
`.github/workflows/pages.yml` as well. Check the Actions run after pushing; a
failed firmware build prevents deployment, so the last working installer stays
online.

## Single-channel hardware test

The diagnostics screen contains a single-channel LEDC test for the original
ScopeBuddy output:

1. Open **Settings → Diagnostics → 1-channel test**.
2. Connect oscilloscope CH1 to `GPIO48` and the probe ground to board `GND`.
3. Start the test on the display.

GPIO48 outputs a 1 kHz signal with 50% duty cycle and an expected level of
approximately 0 to 3.3 V. Stopping or leaving the test drives GPIO48 LOW.

## Dual-channel hardware test

The diagnostics screen contains a two-channel RMT test used to validate the
ScopeBuddy 0.5 output pair:

1. Open **Settings → Diagnostics → 2-channel test**.
2. Connect oscilloscope CH1 to `GPIO48` and CH2 to `GPIO47`.
3. Connect both probe grounds to the same board `GND`.
4. Start the test on the display.

Both channels output a 1 kHz signal with 50% duty cycle. The rising edge on
GPIO47 follows the rising edge on GPIO48 by 100 µs. The two 1 ms timelines are
started by the ESP32-P4 RMT synchronization manager and must not drift relative
to each other.

GPIO47 and GPIO48 share expansion connector functions. Do not attach or enable
an expansion UART/SPI device while running this test.

The test has been verified on the target CrowPanel: both outputs measured
1 kHz at 50% duty cycle, with GPIO47 following GPIO48 by 100 µs as specified.

The diagnostic uses the same general pair-sequence API intended for lessons.
It compiles arbitrary level/duration segments for both channels, rejects pairs
with unequal total duration, starts both RMT channels synchronously, and drives
both pins LOW when the output is stopped.

The general driver has also been verified on the target hardware for stopping
both outputs at LOW and switching back from paired RMT output to an existing
single-channel RMT lesson.

All six ScopeBuddy 0.5 two-channel lessons have been smoke-tested on the target
CrowPanel with an oscilloscope. Trigger/response timing, phase shift, frequency
division, ultrasonic echo timing, gated PWM, and quadrature phase order match
their displayed results. Stopping a lesson leaves both outputs LOW.

## Project structure

- `main/` — ScopeBuddy application and LVGL user interface
- `peripheral/` — display, touch, backlight, I2C, and board support
- `docs/` — GitHub Pages web installer and ESP Web Tools manifest
- `.github/workflows/pages.yml` — reproducible firmware build and Pages deployment
- `tools/` — asset conversion helpers
- `sdkconfig.defaults` — default ESP-IDF configuration
