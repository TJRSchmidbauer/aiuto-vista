# ScopeBuddy

ScopeBuddy is an ESP-IDF/LVGL application for the Elecrow CrowPanel Advance
5-inch ESP32-P4 display. It provides a touch interface and rotary-encoder input
for controlling and visualizing signal-generator settings.

## Hardware

- Elecrow CrowPanel Advance 5-inch ESP32-P4, 800 × 480
- Optional rotary encoder; see [ENCODER.md](ENCODER.md) for wiring

## Build

Install ESP-IDF 5.4 or newer, activate its environment, and run:

```sh
idf.py set-target esp32p4
idf.py build
idf.py flash monitor
```

ESP-IDF's component manager downloads LVGL, the ESP LVGL port, and the display
and touch-controller drivers declared in `main/idf_component.yml`.

## Project structure

- `main/` — ScopeBuddy application and LVGL user interface
- `peripheral/` — display, touch, backlight, I2C, and board support
- `tools/` — asset conversion helpers
- `sdkconfig.defaults` — default ESP-IDF configuration
