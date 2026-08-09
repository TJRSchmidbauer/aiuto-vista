# Aiuto-Vista

Aiuto-Vista turns a Cheap Yellow Display (CYD, ESP32-2432S028) into a practical
oscilloscope trainer. It generates genuine 3.3-V signals, presents randomized
measurement tasks, and optionally reveals the expected values.

**Origin:** Aiuto-Vista is a **fork of ScopeBuddy** by Johannes Börnsen
(https://github.com/johannesboernsen/ScopeBuddy). The idea, the lessons, the
measurement tasks, and large parts of the code come from the original project,
which was developed for the Elecrow CrowPanel Advance 5″ (ESP32-P4). This fork
ports ScopeBuddy to the inexpensive CYD touch display (ESP32) and can only be
understood together with the original.

Firmware 0.7.0 provides 15 single- and dual-channel lessons, touch calibration,
and CYD touchscreen operation. It ships in two variants: the **digital**
variant drives the signals through the GPIO pins, the **analog** variant
outputs them through two MCP4725 DACs on the I2C header.

## What you need

| Component | Requirement | Notes |
|---|---|---|
| Display | Cheap Yellow Display (CYD) `ESP32-2432S028`, 320 × 240, touchscreen | USB-C or Micro-USB, depending on revision |
| Oscilloscope | One channel for the basic lessons, two channels for all lessons | Use high-impedance inputs and 3.3-V-logic-safe probes |
| USB cable | Data cable for flashing the firmware | Charge-only cables do not work |
| MCP4725-DAC variant only | Two MCP4725 breakout boards, breadboard, jumper wires | See [MCP4725-DAC variant](#mcp4725-dac-variant) |

## Wiring (digital variant)

- **CH1 signal:** `GPIO26` (audio in / left pin of the 3.5 mm jack)
- **CH2 signal:** `GPIO27` (CN1 header)
- **Ground:** `GND` pin on the CN1 header (the jack sleeve also carries GND)

GPIO26 and GPIO27 are 3.3-V logic outputs intended only for high-impedance
oscilloscope or logic-analyzer inputs. Do not connect loads or external
voltages to these pins.

## MCP4725-DAC variant

The DAC variant outputs the measurement signals as true analog voltages
through two MCP4725 DACs instead of GPIO26/GPIO27:

| Component | Requirement |
|---|---|
| DAC for CH1 (DAC-A) | MCP4725 with A0 tied to **GND** (address `0x60`) |
| DAC for CH2 (DAC-B) | MCP4725 with A0 tied to **VDD** (address `0x61`) |
| Wiring | SDA to **GPIO22**, SCL to **GPIO23** (CN1 header) |

Power the boards with 3.3 V and GND; a 100 nF decoupling capacitor at the VDD
pin of each board is recommended. The `VOUT` pin delivers 0 V to 3.3 V in 4096
steps and can feed the oscilloscope input directly (a voltage divider is not
required but allowed).

Outputs of the two variants compared:

| Signal | Digital (standard) | Analog (MCP4725-DAC) |
|---|---|---|
| CH1 output | `GPIO26` | `0x60` (A0 to GND) |
| CH2 output | `GPIO27` | `0x61` (A0 to VDD) |
| Wave shape | Square (3.3 V) | Staircase (DAC envelope) |
| Tachometer signal | Square pulses | True sine wave, up to 500 Hz |
| PWM / oscillator | Up to 20 kHz | Up to 2 kHz |
| Logic level | 0 V / 3.3 V | 0 V … 3.3 V, 4096 steps |

The DAC variant reports the same diagnostic signals as the digital variant,
with the tachometer lesson emitting a sine wave instead of pulses.

## Installing

The easiest way is the prebuilt firmware. You need to know your device's
serial port (e.g. `/dev/ttyUSB0` on Linux). The browser installer at
https://TJRSchmidbauer.github.io/aiuto-vista/ asks for the firmware version
and the variant, then transfers the matching image over USB. No ESP-IDF,
Python, or Git is required.

Alternatively, flash the merged image with esptool. The merged image contains
bootloader, partition table, and application in a single step. The firmware
files are generated into `docs/firmware/` by the CI workflow:

Digital variant:

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write-flash -z 0x0 docs/firmware/aiuto-vista-v0.7.0-digital.bin
```

MCP4725-DAC variant:

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write-flash -z 0x0 docs/firmware/aiuto-vista-v0.7.0-mcp4725.bin
```

## First hardware check

Two tests are available under **Einstellungen → Diagnose** (Settings →
Diagnostics):

1. **Single-channel test:** `GPIO26` (or DAC-A) outputs 1 kHz at 50% duty
   cycle.
2. **Two-channel test:** `GPIO26`/`GPIO27` (or DAC-A/DAC-B) output 1 kHz at
   50%, with channel 2 following channel 1 with a 100 µs delay.

The expected measurement values are documented in the
[hardware validation](docs/HARDWARE_VALIDATION.md).

## Touch calibration

Because the XPT2046 touch screens of the CYD boards vary from unit to unit,
the touch can be individually tuned under **Einstellungen → Touch-Kalibrierung**
(Settings → Touch calibration): tap five crosses one after another; the
calibration is stored per device in the ESP32's NVS memory.

## Lessons

A lesson starts an open series of randomized tasks. Aiuto-Vista does not
repeat the ten most recently generated signal configurations of a series. The
confirmation prompt before returning to the start screen can be disabled in
the settings.

Single-channel lessons on GPIO26 (or DAC-A) — German labels as shown in the
UI:

- Periodisches Signal (periodic signal)
- Pulsbreiten (pulse widths)
- Burst (burst)
- Pulslücke (pulse gap)
- Servosignal (servo signal)
- Tachosignal (tachometer signal)
- Tasterprellen (contact bounce)
- UART 8N1 (UART 8N1)
- Zustandswechsel (state change)

Two-channel lessons on GPIO26 and GPIO27 (or DAC-A and DAC-B):

- Trigger-Antwort (trigger response)
- Phasenverschiebung (phase shift)
- Frequenzteiler (frequency divider)
- Ultraschall-Echo (ultrasonic echo)
- Freigegebene PWM (gated PWM)
- Quadraturgeber (quadrature encoder)

## Building from source

Aiuto-Vista 0.7.0 is built with ESP-IDF 5.4.2 and tested on the CYD. Install
and activate ESP-IDF, then:

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

The DAC variant is enabled through an additional configuration file:

```bash
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.mcp4725.defaults" \
  idf.py -B build_mcp4725 build
idf.py -p /dev/ttyUSB0 -B build_mcp4725 flash monitor
```

The Kconfig menu of the `bsp_extra` component controls the DAC variant
(`CYD_MCP4725_SUPPORT`) as well as the I2C pins, the I2C clock frequency, and
the DAC addresses (`CYD_DAC_I2C_SDA`, `CYD_DAC_I2C_SCL`,
`CYD_DAC_I2C_FREQ_HZ`, `CYD_DAC_DAC_A_ADDRESS`, `CYD_DAC_DAC_B_ADDRESS`).

The ESP-IDF component manager downloads the required LVGL, display, and touch
components automatically. `dependencies.lock` pins the versions for release
builds.

## License and legal notices

This fork is based on the **original contributions of ScopeBuddy**, which are
licensed under the [ScopeBuddy Community License 1.0](LICENSE.md). Under that
license you may build, study, modify, and publish the code, including for
education and research. Published modified versions must provide the
corresponding source code, include a **reference to the original project**,
and carry a **distinct name** — this fork satisfies that with the name
Aiuto-Vista and the references to
https://github.com/johannesboernsen/ScopeBuddy.

ScopeBuddy devices and kits may be distributed at cost price. A
profit-oriented sale requires a separate commercial license from the owner of
the original repository. ScopeBuddy — and therefore this fork as well — is
*source-available* but not open source in the sense of the Open Source
Initiative.

Board-support material derived from Elecrow and other third-party components
are not relicensed by ScopeBuddy. Their status and the exact license
boundaries are documented in [LICENSE.md](LICENSE.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Origin and acknowledgments

All lessons, measurement tasks, the UI concept, and essential software parts
originate from **ScopeBuddy by Johannes Börnsen**
(https://github.com/johannesboernsen/ScopeBuddy). Without this project there
would be no Aiuto-Vista. Please support the original project; contributions
are directed to the original and follow its
[CONTRIBUTING.md](CONTRIBUTING.md) with the [CLA](CLA.md) described there.

## Further documentation

- [Hardware validation and diagnostics](docs/HARDWARE_VALIDATION.md)
- [Maintenance and release process](docs/MAINTAINING.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)