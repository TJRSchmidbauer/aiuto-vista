# ScopeBuddy

ScopeBuddy turns an Elecrow CrowPanel Advanced 5-inch ESP32-P4 display into a
hands-on oscilloscope trainer. It generates real 3.3 V digital signals, presents
randomized measurement tasks, and reveals the expected values on demand.

Firmware 0.5.1 provides 15 single- and dual-channel lessons. The display can be
operated by touch or with an optional rotary encoder.

## What you need

| Component | Requirement | References and sources |
| --- | --- | --- |
| Display | Elecrow CrowPanel Advanced 5-inch, ESP32-P4, 800 × 480 (`DHE04005D`) | [Official product page][crowpanel-product] · [Elecrow documentation][crowpanel-wiki] · [Amazon.de][crowpanel-amazon] |
| Oscilloscope | One channel for the basic lessons; two channels for all lessons | Use high-impedance inputs and probes suitable for 3.3 V logic |
| USB cable | USB data cable for installing the firmware | Charge-only cables do not work |
| Rotary encoder (optional) | GIAK KY-040 module or a compatible 3.3 V encoder module | [Wiring guide](ENCODER.md) · [Amazon.de][encoder-amazon] |
| Enclosure (optional) | 3D-printable ScopeBuddy enclosure | [STL model](hardware/enclosure/ScopeBuddy.stl) · [Model notes](hardware/enclosure/README.md) |

The Amazon links are non-affiliate purchase links. Listings and availability can
change; use the model names and electrical requirements above when selecting an
equivalent component.

## Assemble the hardware

1. Disconnect USB and any other power source before wiring the optional rotary
   encoder.
2. Connect the encoder as described in the [wiring guide](ENCODER.md). Power it
   from 3.3 V, not 5 V. The optional radio module uses the same GPIOs and must
   not be installed or enabled at the same time.
3. If desired, print the [enclosure](hardware/enclosure/ScopeBuddy.stl). The STL
   uses millimeters and measures approximately 97 × 159.9 × 46 mm.
4. Connect oscilloscope CH1 to `GPIO48`. For two-channel lessons, connect CH2 to
   `GPIO47`. Connect both probe grounds to board `GND`.

GPIO47 and GPIO48 are 3.3 V logic outputs intended only for high-impedance
oscilloscope or logic-analyzer inputs. Do not connect loads or external voltages
to these pins. The pins also share expansion connector functions; do not attach
an expansion UART or SPI device while ScopeBuddy is generating signals.

## Install ScopeBuddy

The browser installer is the easiest installation method and does not require
ESP-IDF, Python, or Git.

**[Open the ScopeBuddy web installer](https://johannesboernsen.github.io/ScopeBuddy/)**

1. Open the installer in Chrome or Edge on a desktop computer.
2. Connect the CrowPanel with a USB data cable.
3. Hold **BOOT**, briefly press **RST** while still holding **BOOT**, and then
   release **BOOT**. This download-mode sequence is required before every
   connection attempt.
4. Select **Connect device** and choose **ESP32-P4** as the serial port.
5. Select **Install ScopeBuddy**, allow **Erase Device**, and confirm the
   installation.
6. Keep the USB cable connected until the installer displays **Wrapping up**.
7. Briefly press **RST** on the back of the display. ScopeBuddy then starts
   automatically.

The installer supports only the specified CrowPanel ESP32-P4 model. Installing
the firmware erases existing ScopeBuddy settings and other flash data.

If the board is not listed, close other applications using its serial port,
check that the cable supports data, reload the page, and repeat the complete
BOOT/RST sequence. Firefox and Safari do not currently provide the Web Serial
API required by the installer.

## Run the first hardware check

ScopeBuddy includes two diagnostic signals for checking a newly assembled
device and its oscilloscope connections:

1. Open **Settings → Diagnostics → 1-channel test**. GPIO48 should output
   1 kHz at 50% duty cycle.
2. Open **Settings → Diagnostics → 2-channel test**. Both channels should output
   1 kHz at 50% duty cycle, with GPIO47 following GPIO48 by 100 µs.

Stopping a test or leaving its page drives the outputs LOW. Detailed expected
measurements and the validation record are in
[Hardware validation](docs/HARDWARE_VALIDATION.md).

## Lessons

Selecting a lesson starts an open-ended series of randomized tasks. ScopeBuddy
avoids repeating any of the ten most recent signal configurations in that
series. The return-to-start confirmation can be disabled in the settings.

Single-channel lessons on GPIO48:

- Periodic signal
- Pulse widths
- Burst
- Missing pulse
- Servo signal
- Tachometer signal
- Button bounce
- UART 8N1
- Alternating states

Dual-channel lessons on GPIO48 and GPIO47:

- Trigger/response
- Phase shift
- Frequency divider
- Ultrasonic echo
- Gated PWM
- Quadrature encoder

## Build from source

ScopeBuddy 0.5.1 is built and hardware-tested with ESP-IDF 5.4.2. Install and
activate ESP-IDF, then run:

```sh
idf.py set-target esp32p4
idf.py build
idf.py flash monitor
```

ESP-IDF's component manager downloads the required LVGL, display, and touch
components. `dependencies.lock` pins the component versions used for release
builds.

## License

Original ScopeBuddy contributions are available under the
[ScopeBuddy Community License 1.0](LICENSE.md). You may build, study, modify,
and publish them, including for education and research. Published modified
versions must provide their corresponding source, retain a reference to this
project, and use a distinct name.

ScopeBuddy devices and kits may be passed on at direct cost. Selling
them for profit requires a separate commercial license from the repository
owner. ScopeBuddy is therefore source-available, not Open Source as defined by
the Open Source Initiative.

Elecrow-derived board-support material and other third-party components are
not relicensed by ScopeBuddy. Their status and the exact repository license
boundaries are documented in [LICENSE.md](LICENSE.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Contributions are welcome under the process in
[CONTRIBUTING.md](CONTRIBUTING.md). Pull requests must accept the
[Contributor License Agreement](CLA.md), which lets contributors retain their
copyright while allowing ScopeBuddy to continue offering separate commercial
licenses.

## Further documentation

- [Rotary encoder wiring](ENCODER.md)
- [Enclosure model and dimensions](hardware/enclosure/README.md)
- [Hardware diagnostics and validation](docs/HARDWARE_VALIDATION.md)
- [Maintainer and release guide](docs/MAINTAINING.md)

[crowpanel-product]: https://www.elecrow.com/crowpanel-advanced-5inch-esp32-p4-hmi-ai-display-800x480-ips-touch-screen-with-wifi-6.html
[crowpanel-wiki]: https://www.elecrow.com/wiki/CrowPanel_Advanced_5inch_ESP32-P4_HMI_AI_Display_800x480_IPS_Touch_Screen_with_WiFi_6.html
[crowpanel-amazon]: https://amzn.eu/d/075Lfm2j
[encoder-amazon]: https://amzn.eu/d/03NJfjfa
