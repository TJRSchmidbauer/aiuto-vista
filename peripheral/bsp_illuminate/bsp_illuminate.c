/* Aiuto-Vista board support package: LVGL integration and backlight.
 * Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
 */

#include "bsp_illuminate.h"
#include "bsp_display.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

lv_display_t *my_lvgl_disp = NULL;
lv_indev_t *my_touch_indev = NULL;

#ifdef CONFIG_CYD_SW_MIRROR_X
#define CYD_SW_MIRROR_X 1
#else
#define CYD_SW_MIRROR_X 0
#endif

#ifdef CONFIG_CYD_SW_MIRROR_Y
#define CYD_SW_MIRROR_Y 1
#else
#define CYD_SW_MIRROR_Y 0
#endif

/* LVGL flush callback with optional software mirroring.
 * Required because some CYD panels ignore the MADCTL MX/MY bits, so a fixed
 * address inversion remains in the panel's logical-to-physical mapping.
 * A pure in-band pixel swap would leave the flush windows (LVGL flushes the
 * screen in bands) in reversed order. The exact inverse of the panel
 * mapping is: reverse the pixels within the window AND invert the window
 * coordinates before drawing.
 * The mirroring is done in place in the LVGL draw buffer: LVGL only reuses
 * the buffer after the flush-ready signal, so in-flight async SPI transfers
 * can never read overwritten pixels (a shared bounce buffer would break
 * when multiple flushes are queued at once).
 * The flush-ready signal is sent by the panel IO color-trans-done callback
 * (LVGL_PORT_HANDLE_FLUSH_READY), same as the stock LVGL port flush. */
static void lvgl_port_flush_mirror_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2 + 1;
    int y2 = area->y2 + 1;

#if CYD_SW_MIRROR_X || CYD_SW_MIRROR_Y
    if (CYD_SW_MIRROR_X) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w / 2; x++) {
                lv_color_t tmp = color_map[y * w + x];
                color_map[y * w + x] = color_map[y * w + (w - 1 - x)];
                color_map[y * w + (w - 1 - x)] = tmp;
            }
        }
        x1 = H_size - x2;
        x2 = H_size - area->x1;
    }
    if (CYD_SW_MIRROR_Y) {
        for (int y = 0; y < h / 2; y++) {
            for (int x = 0; x < w; x++) {
                lv_color_t tmp = color_map[y * w + x];
                color_map[y * w + x] = color_map[(h - 1 - y) * w + x];
                color_map[(h - 1 - y) * w + x] = tmp;
            }
        }
        y1 = V_size - y2;
        y2 = V_size - area->y1;
    }
#endif

    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, color_map);
}

/* Backlight: LEDC PWM on GPIO21 */
static esp_err_t blight_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = BLIGHT_PWM_Hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), ILLUMINATE_TAG, "Backlight timer config failed");

    const ledc_channel_config_t channel_cfg = {
        .gpio_num = LCD_GPIO_BLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&channel_cfg);
}

esp_err_t set_lcd_blight(uint32_t brightness)
{
    if (brightness > 100) {
        brightness = 100;
    }
    const uint32_t duty = (brightness * ((1U << BLIGHT_PWM_BITS) - 1U)) / 100U;
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    }
    return ret;
}

static void project_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    if (touch_read() == ESP_OK) {
        get_coor(&x, &y, &pressed);
    }
    if (pressed) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static esp_err_t lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = configMAX_PRIORITIES - 4,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 10,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), ILLUMINATE_TAG, "LVGL port init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = panel_io_handle,
        .panel_handle = panel_handle,
        .buffer_size = H_size * 40,
        .double_buffer = false,
        .hres = H_size,
        .vres = V_size,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .full_refresh = false,
            .direct_mode = false,
        },
    };
    my_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    if (my_lvgl_disp == NULL) {
        ILLUMINATE_ERROR("Failed to add LVGL display");
        return ESP_FAIL;
    }
    my_lvgl_disp->driver->flush_cb = lvgl_port_flush_mirror_cb;

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = my_lvgl_disp;
    indev_drv.read_cb = project_touchpad_read;
    my_touch_indev = lv_indev_drv_register(&indev_drv);
    if (my_touch_indev == NULL) {
        ILLUMINATE_ERROR("Failed to add LVGL touch input device");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t display_init(void)
{
    ESP_RETURN_ON_ERROR(blight_init(), ILLUMINATE_TAG, "Backlight init failed");
    ESP_RETURN_ON_ERROR(display_port_init(), ILLUMINATE_TAG, "Display port init failed");
    ESP_RETURN_ON_ERROR(lvgl_init(), ILLUMINATE_TAG, "LVGL init failed");
    ILLUMINATE_INFO("Display ready: %dx%d", H_size, V_size);
    return ESP_OK;
}
