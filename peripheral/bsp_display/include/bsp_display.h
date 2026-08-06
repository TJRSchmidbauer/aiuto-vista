#ifndef _BSP_DISPLAY_H_
#define _BSP_DISPLAY_H_

/* Aiuto-Vista board support package: display and touch.
 * Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
 *
 * Target hardware: Cheap Yellow Display ESP32-2432S028 (ESP32-WROOM-32,
 * ILI9341 or ST7789 panel, XPT2046 resistive touch, CH340C USB bridge).
 * The display controller is detected automatically at boot; use the
 * CYD_* Kconfig options to adjust the fallback and touch mapping.
 */

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"

#define DISPLAY_TAG "DISPLAY"

/* Logging macros for the DISPLAY tag */
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_WARN(fmt, ...) ESP_LOGW(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)

/* Display resolution in landscape orientation */
#define DISPLAY_W 320
#define DISPLAY_H 240

/* TFT panel on SPI bus 2 (HSPI) */
#define LCD_GPIO_SCLK  14
#define LCD_GPIO_MOSI  13
#define LCD_GPIO_MISO  12
#define LCD_GPIO_CS    15
#define LCD_GPIO_DC    2
#define LCD_GPIO_RST   -1 /* panel reset is tied to EN on the CYD */
#define LCD_SPI_FREQ_HZ (40 * 1000 * 1000)

/* XPT2046 resistive touch panel on SPI bus 3 (VSPI) */
#define TOUCH_GPIO_CLK  25
#define TOUCH_GPIO_MOSI 32
#define TOUCH_GPIO_MISO 39
#define TOUCH_GPIO_CS   33
#define TOUCH_GPIO_IRQ  36
#define TOUCH_SPI_FREQ_HZ (2 * 1000 * 1000)

/* LCD panel handle used by the LVGL port (created in display_port_init) */
extern esp_lcd_panel_handle_t panel_handle;
/* LCD panel IO handle used by the LVGL port (created in display_port_init) */
extern esp_lcd_panel_io_handle_t panel_io_handle;
/* Touch panel handle used by the LVGL port (created in touch_init) */
extern esp_lcd_touch_handle_t tp;

/* Public API: get the latest touch coordinates and press state */
void get_coor(uint16_t *x, uint16_t *y, bool *press);
/* Internal API: update the stored touch state (used by touch_read) */
void set_coor(uint16_t x, uint16_t y, bool press);
/* Initialize the SPI LCD panel (controller auto-detection) */
esp_err_t display_port_init(void);
/* Initialize the XPT2046 touch panel */
esp_err_t touch_init(void);
/* Read the touch panel data and update coordinates (calibrated unless raw mode) */
esp_err_t touch_read(void);
/* Store the touch calibration affine transform on this device (NVS).
 * params: tx = p[0]*rx + p[1]*ry + p[2]; ty = p[3]*rx + p[4]*ry + p[5] */
esp_err_t touch_calibration_save(const float params[6]);
/* Load the stored calibration; returns the built-in default if none stored.
 * Pass NULL to just load/refresh the internal state. */
esp_err_t touch_calibration_load(float params[6]);
/* When enabled, touch_read delivers raw (uncalibrated) coordinates.
 * Used by the calibration UI to capture raw touch points. */
void touch_set_raw_mode(bool enabled);

#endif /* _BSP_DISPLAY_H_ */
