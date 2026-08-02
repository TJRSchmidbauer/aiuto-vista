# ScopeBuddy rotary encoder

The encoder is registered as an LVGL encoder input device. Turning it moves
the focus between controls; pressing it activates the focused control. On
editable controls, pressing toggles between navigation and editing.

## Wiring

Always disconnect USB and any other power source before wiring.

| Encoder | CrowPanel signal |
| --- | --- |
| `CLK` | `IO29` |
| `DT` | `IO30` |
| `SW` | `IO32` |
| `+` | `3V3` |
| `GND` | `GND` |

Use 3.3 V, not 5 V. The encoder signal pins are pulled up internally. The
optional radio module shares these GPIOs and must not be enabled or connected
at the same time.

If the direction feels reversed, swap the `CLK` and `DT` wires. The pin
assignments can also be changed in `main/include/encoder_input.h`.
