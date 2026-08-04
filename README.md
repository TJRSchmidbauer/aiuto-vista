# ScopeBuddy

ScopeBuddy is an ESP-IDF/LVGL application for the Elecrow CrowPanel Advance
5-inch ESP32-P4 display. It provides a touch interface and rotary-encoder input
for practical oscilloscope measurement exercises. Firmware 0.5.1 generates
repeatable digital signals and reveals the expected measurements on demand.

## Install from the browser

The easiest installation method does not require ESP-IDF, Python, or Git:

**[Open the ScopeBuddy web installer](https://johannesboernsen.github.io/ScopeBuddy/)**

1. Open the installer in Chrome or Edge on a desktop computer.
2. Connect an Elecrow CrowPanel Advance 5-inch ESP32-P4 with a USB data cable.
3. Put the CrowPanel into download mode every time before connecting: hold
   **BOOT**, briefly press **RST** while continuing to hold **BOOT**, and then
   release **BOOT**.
4. Select **Connect device** and choose **ESP32-P4** as the serial port.
5. Select **Install ScopeBuddy**.
6. Allow **Erase Device** and confirm the installation.
7. Keep the USB cable connected and wait for the installation to finish.
8. When the installer displays **Wrapping up**, briefly press **RST** once on
   the back of the display.
9. ScopeBuddy starts on the display and the installation is complete.

The installer is intended only for the Elecrow CrowPanel Advance 5-inch with
ESP32-P4. A browser installation resets settings stored by an earlier ScopeBuddy
installation; selecting **Erase device** additionally removes all remaining
flash data. Firefox and Safari do not currently expose the Web Serial API
required by the installer.

The CrowPanel is only detected by the browser while it is in download mode; the
BOOT/RST sequence above is mandatory, not merely a troubleshooting fallback. If
the board is still not listed, close other programs using its serial port, try
another USB data cable, reload the page, and repeat the complete button sequence
before selecting **Connect device** again.

The CrowPanel does not reliably perform the final automatic reset requested by
ESP Web Tools. This is why pressing **RST** at **Wrapping up** is a normal,
board-specific part of the browser installation and does not indicate a failed
flash operation.

## Lessons

ScopeBuddy 0.5.1 includes 15 lessons. Each tile starts an open-ended series of
randomized measurement challenges that continues until the user returns to the
start screen. Nine lessons are single-channel exercises:

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

Each series avoids matching any of its ten most recently generated signal
signatures. Returning to the start screen normally requires confirmation; this
confirmation can be disabled in the settings.

The continuous PWM lessons use the ESP32-P4 LEDC peripheral. Finite and
irregular pulse sequences use the RMT peripheral at 1 MHz resolution so their
displayed timing values match the generated hardware sequence.

## Hardware

| Component | Model or requirement | References and sources |
| --- | --- | --- |
| Display | Elecrow CrowPanel Advanced 5-inch, ESP32-P4, 800 × 480 (`DHE04005D`) | [Official product page][crowpanel-product] · [Elecrow documentation][crowpanel-wiki] · [Amazon.de][crowpanel-amazon] |
| Rotary encoder (optional) | GIAK KY-040 module, operated at 3.3 V | [Wiring](ENCODER.md) · [Amazon.de][encoder-amazon] |
| Enclosure (optional) | ScopeBuddy 3D-printable enclosure | [STL model](hardware/enclosure/ScopeBuddy.stl) · [Model notes](hardware/enclosure/README.md) |

The Amazon links are non-affiliate purchase links. Product listings and
availability can change; use the model names and electrical requirements above
when selecting an equivalent component.

Connect oscilloscope CH1 to `GPIO48`, CH2 to `GPIO47`, and both grounds to
board `GND` for two-channel lessons.

The enclosure STL uses millimeters and measures approximately
97 × 159.9 × 46 mm. Import it into a slicer without scaling; print orientation,
supports, and material settings depend on the printer and filament in use.

GPIO47 and GPIO48 are 3.3 V logic outputs intended for high-impedance
oscilloscope or logic-analyzer inputs. Do not connect loads or external
voltages to them.

[crowpanel-product]: https://www.elecrow.com/crowpanel-advanced-5inch-esp32-p4-hmi-ai-display-800x480-ips-touch-screen-with-wifi-6.html
[crowpanel-wiki]: https://www.elecrow.com/wiki/CrowPanel_Advanced_5inch_ESP32-P4_HMI_AI_Display_800x480_IPS_Touch_Screen_with_WiFi_6.html
[crowpanel-amazon]: https://amzn.eu/d/075Lfm2j
[encoder-amazon]: https://amzn.eu/d/03NJfjfa

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

ScopeBuddy 0.5.1 is built and hardware-tested with ESP-IDF 5.4.2.

## Web installer maintenance

The static installer lives in `docs/`. `docs/versions.json` is the version
catalog used by the version selector. The installer creates the matching ESP
Web Tools manifest in the browser and points it to the selected merged firmware
image. On every push to `main`, GitHub Actions:

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

Keep these version values in sync for a release:

- `PROJECT_VER` in `CMakeLists.txt`
- `SCOPEBUDDY_FIRMWARE_VERSION` in `main/ui/ui_events.c`
- `version` and the firmware filename in `docs/manifest.json`
- `current`, `version`, and the firmware filename in `docs/versions.json`
- the firmware filename in `.github/workflows/pages.yml`

The Pages workflow currently publishes version 0.5.1 from `main`. Check the
Actions run after pushing; a failed firmware build prevents deployment, so the
last working installer stays online.

Tested releases are archived independently of GitHub Pages. Create and push a
version tag only after the firmware has been accepted on the target hardware:

```sh
git tag -a v0.5.1 -m "ScopeBuddy 0.5.1"
git push origin v0.5.1
```

`.github/workflows/release.yml` verifies that the tag matches the versions in
the source code, builds the firmware, and publishes the merged image and its
SHA-256 checksum as versioned GitHub Release assets.

When publishing the next version, keep the new current entry at the top of
`docs/versions.json` and retain the previous entries. Change an older entry's
`firmware` value to its permanent release URL, for example:

```json
{
  "version": "0.5.1",
  "recommended": false,
  "firmware": "https://github.com/johannesboernsen/ScopeBuddy/releases/download/v0.5.1/scopebuddy-v0.5.1.bin"
}
```

Only the current entry refers to the firmware generated by the Pages workflow.
This keeps historical downloads available even though each Pages deployment
replaces the previous site artifact.

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
ScopeBuddy 0.5.1 output pair:

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

All six ScopeBuddy 0.5.1 two-channel lessons have been smoke-tested on the target
CrowPanel with an oscilloscope. Trigger/response timing, phase shift, frequency
division, ultrasonic echo timing, gated PWM, and quadrature phase order match
their displayed results. Stopping a lesson leaves both outputs LOW.

## Project structure

- `main/` — ScopeBuddy application and LVGL user interface
- `hardware/enclosure/` — printable ScopeBuddy enclosure model
- `peripheral/` — display, touch, backlight, I2C, and board support
- `docs/` — GitHub Pages web installer and ESP Web Tools manifest
- `.github/workflows/pages.yml` — reproducible firmware build and Pages deployment
- `tools/` — asset conversion helpers
- `sdkconfig.defaults` — default ESP-IDF configuration
