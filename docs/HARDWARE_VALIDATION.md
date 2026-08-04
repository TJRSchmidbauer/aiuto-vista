# ScopeBuddy hardware validation

This document records the diagnostic signals and target-hardware checks used for
ScopeBuddy firmware releases.

## Connections and electrical limits

- CH1 signal: `GPIO48`
- CH2 signal: `GPIO47`
- Probe grounds: common board `GND`
- Output levels: approximately 0 to 3.3 V

GPIO47 and GPIO48 are intended for high-impedance oscilloscope or logic-analyzer
inputs. Do not connect loads or external voltages. They share expansion connector
functions, so an expansion UART or SPI device must not be attached or enabled
while a diagnostic or lesson is running.

## Single-channel diagnostic

1. Open **Settings → Diagnostics → 1-channel test**.
2. Connect oscilloscope CH1 to `GPIO48` and the probe ground to board `GND`.
3. Start the test.

Expected result: GPIO48 outputs 1 kHz at 50% duty cycle. Stopping or leaving the
test drives GPIO48 LOW.

## Dual-channel diagnostic

1. Open **Settings → Diagnostics → 2-channel test**.
2. Connect CH1 to `GPIO48`, CH2 to `GPIO47`, and both probe grounds to board
   `GND`.
3. Start the test.

Expected result: both channels output 1 kHz at 50% duty cycle. The rising edge on
GPIO47 follows the rising edge on GPIO48 by 100 µs. The two 1 ms timelines are
started by the ESP32-P4 RMT synchronization manager and must not drift relative
to each other. Stopping or leaving the test drives both outputs LOW.

## Firmware 0.5.1 validation record

The following checks were performed on the target Elecrow CrowPanel Advanced
5-inch ESP32-P4 with an oscilloscope:

- Single-channel diagnostic: 1 kHz and 50% duty cycle on GPIO48
- Dual-channel diagnostic: 1 kHz and 50% duty cycle on both outputs, with GPIO47
  following GPIO48 by 100 µs
- Both outputs stop at LOW
- Switching from paired RMT output back to an existing single-channel RMT lesson
- All six dual-channel lessons: trigger/response, phase shift, frequency divider,
  ultrasonic echo, gated PWM, and quadrature encoder

The measured timing, phase order, and calculated results matched the values shown
by ScopeBuddy.

The paired-output driver compiles arbitrary level/duration segments, rejects
pairs with unequal total duration, starts both RMT channels synchronously, and
drives both pins LOW when stopped. Continuous PWM lessons use the ESP32-P4 LEDC
peripheral; finite and irregular sequences use RMT at 1 MHz resolution.
