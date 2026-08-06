# Aiuto-Vista hardware validation

This document describes how to validate an Aiuto-Vista build on the target
Cheap Yellow Display (ESP32-2432S028). Aiuto-Vista is a fork of ScopeBuddy
(https://github.com/johannesboernsen/ScopeBuddy).

## Signal pins

- CH1 signal: `GPIO26` (audio in- / left pin of the 3.5 mm jack)
- CH2 signal: `GPIO27` (CN1 header)
- Ground reference: `GND` pin on the CN1 header (the jack sleeve also carries GND)

GPIO26 and GPIO27 are driven as push-pull outputs (0 to 3.3 V) and are intended
for high-impedance oscilloscope or logic-analyzer probes.

## Single-channel diagnostic

1. Flash the firmware and let it boot to the start screen.
2. Connect oscilloscope CH1 to `GPIO26` and the probe ground to board `GND`.
3. Open Einstellungen → Diagnose → Hardwaretests and start the 1-channel test.

Expected result: GPIO26 outputs 1 kHz at 50% duty cycle. Stopping or leaving the
test drives GPIO26 LOW.

## Two-channel diagnostic

1. Connect CH1 to `GPIO26`, CH2 to `GPIO27`, and both probe grounds to board
   `GND`.
2. Start the 2-channel test from the hardware tests screen.

Expected result: both channels output 1 kHz at 50% duty cycle, and the rising
edge on GPIO27 follows the rising edge on GPIO26 by 100 µs.

## Lesson smoke tests

Run at least the first lesson of every category (1-channel PWM, two-channel
sequences, alternating signals, ultrasonic, quadrature). For each lesson:

1. Verify the connection hints match the wiring above.
2. Verify the challenge signal appears on the announced pin.
3. Verify a fresh challenge is generated for each "Nächste Messaufgabe".
4. Verify the "Auto-Vorbereitung" reference signal appears before each task.

## Recorded checks

The following checks were performed on the target ESP32-2432S028:

- Single-channel diagnostic: 1 kHz and 50% duty cycle on GPIO26
- Two-channel diagnostic: 1 kHz on GPIO26 and GPIO27, GPIO27 rising edge
  following GPIO26 by 100 µs
- Backlight control, touch input, and all screen transitions

A release should only be published after these checks have passed on a current
build.
