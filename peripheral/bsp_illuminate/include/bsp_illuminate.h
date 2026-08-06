#ifndef _BSP_ILLUMINATE_H_
#define _BSP_ILLUMINATE_H_

/* Aiuto-Vista board support package: LVGL integration and backlight.
 * Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
 *
 * The display backlight of the Cheap Yellow Display is driven by a PWM
 * signal on GPIO21 (LEDC channel). LVGL runs with the esp_lvgl_port
 * library on top of the SPI panel and touch handles from bsp_display.
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"
#include "driver/ledc.h"
#include "lvgl.h"

#define ILLUMINATE_TAG "ILLUMINATE"

#define ILLUMINATE_INFO(fmt, ...) ESP_LOGI(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_ERROR(fmt, ...) ESP_LOGE(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)

/* Display resolution (landscape) */
#define H_size 320
#define V_size 240
#define BITS_PER_PIXEL 16

/* Backlight: PWM on GPIO21 (LEDC, low speed mode, 8 bit resolution) */
#define LCD_GPIO_BLIGHT 21
#define BLIGHT_PWM_Hz 5000
#define BLIGHT_PWM_BITS 8

/* LVGL display and input device handles (used by the UI) */
extern lv_display_t *my_lvgl_disp;
extern lv_indev_t *my_touch_indev;

/* Initialize the backlight PWM, the display and LVGL */
esp_err_t display_init(void);
/* Set the display backlight brightness (0..100 percent) */
esp_err_t set_lcd_blight(uint32_t brightness);

#endif /* _BSP_ILLUMINATE_H_ */
