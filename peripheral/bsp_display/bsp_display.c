/* Aiuto-Vista board support package: display and touch driver.
 * Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
 */

#include "bsp_display.h"

#include <string.h>
#include "nvs_flash.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_st7789.h"
#include "driver/spi_master.h"

esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t panel_io_handle = NULL;
esp_lcd_touch_handle_t tp = NULL;

static uint16_t touch_x = 0xffff;
static uint16_t touch_y = 0xffff;
static bool is_pressed = false;

/* Touch calibration (XPT2046 overlay on this panel): affine transform from
 * raw driver coordinates to display coordinates, stored per device in NVS.
 * Default is the linear fit derived from corner taps:
 *   tx = 321.8 - 1.395*ry;  ty = 237.3 - 0.737*rx
 * (raw channel X runs along the display's SHORT axis inverted, raw channel Y
 * along the LONG axis inverted; the driver axis flags are all disabled). */
#define CAL_NVS_NAMESPACE "touch_cal"
#define CAL_NVS_KEY       "affine"

static float cal_params[6];
static bool cal_loaded = false;
static bool raw_mode = false;

static void apply_default_calibration(void)
{
    cal_params[0] = 0.0f;
    cal_params[1] = -1.395f;
    cal_params[2] = 321.8f;
    cal_params[3] = -0.737f;
    cal_params[4] = 0.0f;
    cal_params[5] = 237.3f;
}

typedef enum {
    DISPLAY_ILI9341,
    DISPLAY_ST7789
} display_controller_t;

static display_controller_t display_controller = DISPLAY_ST7789;

void get_coor(uint16_t *x, uint16_t *y, bool *press)
{
    *x = touch_x;
    *y = touch_y;
    *press = is_pressed;
}

void set_coor(uint16_t x, uint16_t y, bool press)
{
    touch_x = x;
    touch_y = y;
    is_pressed = press;
}

/* Read the panel controller ID (RDDID, command 0x04) via a raw SPI device.
 * Returns 0 if the ID could not be determined (fallback is used then). */
static uint32_t read_panel_id(void)
{
    uint32_t id = 0;
    spi_device_handle_t spid = NULL;

    /* The raw SPI device does not control the DC line; pull it low so the
     * transaction is seen as a command by the panel controller. */
    gpio_set_direction(LCD_GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_GPIO_DC, 0);

    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_GPIO_SCLK,
        .mosi_io_num = LCD_GPIO_MOSI,
        .miso_io_num = LCD_GPIO_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8,
    };
    const spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 1000000,
        .spics_io_num = LCD_GPIO_CS,
        .queue_size = 1,
    };

    if (spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO) != ESP_OK) {
        return 0;
    }
    if (spi_bus_add_device(SPI2_HOST, &dev_cfg, &spid) != ESP_OK) {
        spi_bus_free(SPI2_HOST);
        return 0;
    }

    /* Command byte followed by dummy clocks while the controller drives MISO */
    uint8_t tx_data[8] = { 0x04, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t rx_data[8] = { 0 };
    spi_transaction_t trans = {
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
        .length = 8 * 8,
        .rxlength = 8 * 8,
    };
    if (spi_device_transmit(spid, &trans) != ESP_OK) {
        id = 0;
    } else {
        /* Response layout: dummy byte(s) first, then the controller ID bytes
         * (ILI9341: 0x93, ST7789: 0x85). Scan for known IDs to be robust
         * against differing dummy byte counts. */
        for (size_t i = 1; i < sizeof(rx_data); i++) {
            if (rx_data[i] == 0x93) {
                id = 0x93;
                break;
            }
            if (rx_data[i] == 0x85) {
                id = 0x85;
                break;
            }
        }
    }
    spi_bus_remove_device(spid);
    spi_bus_free(SPI2_HOST);

    /* Let the panel IO driver re-configure the DC pin later */
    gpio_set_direction(LCD_GPIO_DC, GPIO_MODE_INPUT);
    return id;
}

#ifdef CONFIG_CYD_TOUCH_MIRROR_X
#define CYD_TOUCH_MIRROR_X 1
#else
#define CYD_TOUCH_MIRROR_X 0
#endif

#ifdef CONFIG_CYD_TOUCH_SWAP_XY
#define CYD_TOUCH_SWAP_XY 1
#else
#define CYD_TOUCH_SWAP_XY 0
#endif

#ifdef CONFIG_CYD_TOUCH_MIRROR_Y
#define CYD_TOUCH_MIRROR_Y 1
#else
#define CYD_TOUCH_MIRROR_Y 0
#endif

static display_controller_t detect_display_controller(void)
{
    const uint32_t id = read_panel_id();
    DISPLAY_INFO("Panel ID read: 0x%06x", (unsigned)id);

    if ((id & 0x0000ff00) == 0x00009300 || (id & 0x000000ff) == 0x41) {
        return DISPLAY_ILI9341;
    }
    if ((id & 0x0000ff00) == 0x00007700 || (id & 0x000000ff) == 0x85) {
        return DISPLAY_ST7789;
    }

    DISPLAY_WARN("Panel ID unknown, using configured fallback");
#ifdef CONFIG_CYD_DISPLAY_FALLBACK_ILI9341
    return DISPLAY_ILI9341;
#else
    return DISPLAY_ST7789;
#endif
}

esp_err_t display_port_init(void)
{
    esp_err_t ret = ESP_OK;

    display_controller = detect_display_controller();
    DISPLAY_INFO("Detected display controller: %s",
                 display_controller == DISPLAY_ILI9341 ? "ILI9341" : "ST7789");

    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = LCD_GPIO_SCLK,
        .mosi_io_num = LCD_GPIO_MOSI,
        .miso_io_num = LCD_GPIO_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_W * 40 * 2,
    };
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        DISPLAY_ERROR("SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_GPIO_DC,
        .cs_gpio_num = LCD_GPIO_CS,
        .pclk_hz = LCD_SPI_FREQ_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &panel_io_handle);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Panel IO init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* CYD panels are connected with a BGR color order */
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_GPIO_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };
    if (display_controller == DISPLAY_ILI9341) {
        ret = esp_lcd_new_panel_ili9341(panel_io_handle, &panel_cfg, &panel_handle);
    } else {
        ret = esp_lcd_new_panel_st7789(panel_io_handle, &panel_cfg, &panel_handle);
    }
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Panel reset failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Panel setup failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Landscape orientation (like TFT_eSPI/LovyanGFX rotation 1).
     * Some CYD panels do not react to the MADCTL mirror bits (MX/MY);
     * those mirrors are applied in software by the LVGL flush callback
     * (see CYD_SW_MIRROR_X / CYD_SW_MIRROR_Y).
     *   ILI9341: 0x28 = MV | BGR
     *   ST7789:  0x60 = MV | MX (RGB) */
    const uint8_t madctl = (display_controller == DISPLAY_ILI9341) ? 0x28 : 0x60;
    ret = esp_lcd_panel_io_tx_param(panel_io_handle, 0x36, &madctl, 1);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Set MADCTL failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Some ST7789 panels show inverted colors (dark instead of bright) */
    if (display_controller == DISPLAY_ST7789) {
#ifdef CONFIG_CYD_ST7789_INVERT
        const uint8_t invon = 0x21;
        ret = esp_lcd_panel_io_tx_param(panel_io_handle, invon, NULL, 0);
        if (ret != ESP_OK) {
            DISPLAY_ERROR("Set color inversion failed: %s", esp_err_to_name(ret));
            return ret;
        }
#endif
    }

    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Panel display on failed: %s", esp_err_to_name(ret));
        return ret;
    }
    DISPLAY_INFO("Display initialized: %dx%d", DISPLAY_W, DISPLAY_H);
    return ESP_OK;
}

esp_err_t touch_init(void)
{
    esp_err_t ret = ESP_OK;

    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = TOUCH_GPIO_CLK,
        .mosi_io_num = TOUCH_GPIO_MOSI,
        .miso_io_num = TOUCH_GPIO_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Touch SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = -1, /* XPT2046 has no DC line */
        .cs_gpio_num = TOUCH_GPIO_CS,
        .pclk_hz = TOUCH_SPI_FREQ_HZ,
        .lcd_cmd_bits = 8,  /* XPT2046 register byte is sent as SPI command */
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 4,
    };
    esp_lcd_panel_io_handle_t tp_io = NULL;
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_cfg, &tp_io);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("Touch IO init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = DISPLAY_W,
        .y_max = DISPLAY_H,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_GPIO_IRQ,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = CYD_TOUCH_SWAP_XY,
            .mirror_x = CYD_TOUCH_MIRROR_X,
            .mirror_y = CYD_TOUCH_MIRROR_Y,
        },
    };
    ret = esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &tp);
    if (ret != ESP_OK) {
        DISPLAY_ERROR("XPT2046 init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    DISPLAY_INFO("Touch initialized (XPT2046), flags: swap=%d mirror_x=%d mirror_y=%d",
                 CYD_TOUCH_SWAP_XY, CYD_TOUCH_MIRROR_X, CYD_TOUCH_MIRROR_Y);
    return ESP_OK;
}

esp_err_t touch_calibration_load(float params[6])
{
    esp_err_t err = ESP_ERR_NVS_NOT_FOUND;
    nvs_handle_t handle;
    if (nvs_open(CAL_NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
        size_t len = sizeof(cal_params);
        err = nvs_get_blob(handle, CAL_NVS_KEY, cal_params, &len);
        nvs_close(handle);
        if (err != ESP_OK || len != sizeof(cal_params)) {
            err = ESP_ERR_NVS_NOT_FOUND;
            apply_default_calibration();
        }
    } else {
        apply_default_calibration();
    }
    cal_loaded = true;
    if (params != NULL) {
        memcpy(params, cal_params, sizeof(cal_params));
    }
    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
}

esp_err_t touch_calibration_save(const float params[6])
{
    if (params == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(cal_params, params, sizeof(cal_params));
    cal_loaded = true;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CAL_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(handle, CAL_NVS_KEY, cal_params, sizeof(cal_params));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

void touch_set_raw_mode(bool enabled)
{
    raw_mode = enabled;
}

esp_err_t touch_read(void)
{
    if (tp == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = esp_lcd_touch_read_data(tp);
    if (ret != ESP_OK) {
        return ret;
    }
    esp_lcd_touch_point_data_t point;
    uint8_t point_cnt = 0;
    if (esp_lcd_touch_get_data(tp, &point, &point_cnt, 1) == ESP_OK && point_cnt > 0) {
        if (raw_mode) {
            set_coor(point.x, point.y, true);
        } else {
            if (!cal_loaded) {
                touch_calibration_load(NULL);
            }
            int32_t cx = (int32_t)(cal_params[0] * point.x + cal_params[1] * point.y + cal_params[2]);
            int32_t cy = (int32_t)(cal_params[3] * point.x + cal_params[4] * point.y + cal_params[5]);
            if (cx < 0) cx = 0;
            if (cy < 0) cy = 0;
            if (cx > DISPLAY_W - 1) cx = DISPLAY_W - 1;
            if (cy > DISPLAY_H - 1) cy = DISPLAY_H - 1;
            set_coor((uint16_t)cx, (uint16_t)cy, true);
        }
    } else {
        set_coor(0xffff, 0xffff, false);
    }
    return ESP_OK;
}
