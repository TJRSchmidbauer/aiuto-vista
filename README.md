# ScopeBuddy

ScopeBuddy is an ESP-IDF/LVGL application for the Elecrow CrowPanel Advance
5-inch ESP32-P4 display. It provides a touch interface and rotary-encoder input
for practical oscilloscope measurement exercises. Firmware 0.4 generates
repeatable digital signals and reveals the expected measurements on demand.

## Lessons

ScopeBuddy 0.4 includes nine lessons with three difficulty levels each:

- Periodic signal: frequency, period, and duty cycle
- Pulse widths: HIGH time, LOW time, and duty cycle
- Burst: pulse count, edge-to-edge burst duration, and LOW pause
- Missing pulse: base period and enlarged edge interval
- Servo signal: repetition period, pulse width, and target angle
- Tachometer signal: pulse frequency and rotational speed
- Button bounce: edge count and bounce duration
- UART 8N1: bit time, nominal baud rate, and data byte
- Alternating states: two frequencies and their state duration

The continuous PWM lessons use the ESP32-P4 LEDC peripheral. Finite and
irregular pulse sequences use the RMT peripheral at 1 MHz resolution so their
displayed timing values match the generated hardware sequence.

## Hardware

- Elecrow CrowPanel Advance 5-inch ESP32-P4, 800 × 480
- Optional rotary encoder; see [ENCODER.md](ENCODER.md) for wiring
- Oscilloscope input connected to `GPIO48` and board `GND`

GPIO48 is a 3.3 V logic output intended for a high-impedance oscilloscope or
logic-analyzer input. Do not connect loads or external voltages to it.

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

ScopeBuddy 0.4 is built and hardware-tested with ESP-IDF 5.4.2.

## Dual-channel hardware test

The diagnostics screen contains a two-channel RMT test used to validate the
planned ScopeBuddy 0.5 output pair:

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

## Project structure

- `main/` — ScopeBuddy application and LVGL user interface
- `peripheral/` — display, touch, backlight, I2C, and board support
- `tools/` — asset conversion helpers
- `sdkconfig.defaults` — default ESP-IDF configuration
