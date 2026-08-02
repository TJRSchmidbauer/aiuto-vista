// main.c
#include "main.h"
#include "ui.h"
#include "encoder_input.h"
#include "esp_system.h"

static bool wave_hardware_ready;

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external pin";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic/assert";
        case ESP_RST_INT_WDT: return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_WDT: return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "SDIO";
        case ESP_RST_USB: return "USB";
        case ESP_RST_JTAG: return "JTAG";
        case ESP_RST_EFUSE: return "eFuse error";
        case ESP_RST_PWR_GLITCH: return "power glitch";
        case ESP_RST_CPU_LOCKUP: return "CPU lockup";
        default: return "unknown";
    }
}

/**
 • @brief Initialization failure handler (repeatedly prints error information)
 */
static void init_fail_handler(const char *module_name, esp_err_t err) {
    while (1) {  // Infinite loop to help debug which module failed to initialize
        MAIN_ERROR("[%s] init failed: %s", module_name, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 • @brief System initialization (LDO + LCD + backlight + other hardware)

 */
static void system_init(void) {
    esp_err_t err = ESP_OK;

    // 1. Initialize I2C (required for touch chip)
    MAIN_INFO("Initializing I2C...");
    err = i2c_init();
    if (err != ESP_OK) init_fail_handler("I2C", err);
    MAIN_INFO("I2C init success");

    vTaskDelay(200 / portTICK_PERIOD_MS);

    // 2. Initialize stc8 
    err = stc8_i2c_init();
    if (err != ESP_OK)
        init_fail_handler("stc8h1kxx", err);
    MAIN_INFO("stc8 init success");  // Print success log

    // 3. Initialize touch panel (low-level driver)
    MAIN_INFO("Initializing touch panel...");
    err = touch_init();
    if (err != ESP_OK) init_fail_handler("Touch", err);
    MAIN_INFO("Touch panel init success");

    // 4. Initialize LCD hardware and LVGL (must initialize before turning on backlight)
    err = display_init();
    if (err != ESP_OK) init_fail_handler("LCD", err);
    MAIN_INFO("LCD init success");

    // 5. Turn on LCD backlight (brightness set to 100 = max)
    err = set_lcd_blight(100);
    if (err != ESP_OK) init_fail_handler("LCD Backlight", err);
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    // 6. Initialize hardware PWM on GPIO48, initially stopped
    MAIN_INFO("Initializing GPIO48 waveform generator...");
    err = gpio_wave_init(1000, 50);
    if (err == ESP_OK) err = gpio_wave_stop();
    wave_hardware_ready = (err == ESP_OK);
    if (wave_hardware_ready) {
        MAIN_INFO("Waveform generator ready (1000 Hz, 50%% duty, stopped)");
    } else {
        MAIN_ERROR("Waveform generator unavailable: %s", esp_err_to_name(err));
    }

}

void app_main(void)
{
    MAIN_INFO("Starting LED control application...");
    const esp_reset_reason_t reset_reason = esp_reset_reason();
    MAIN_INFO("Previous reset reason: %s (%d)",
              reset_reason_name(reset_reason), (int)reset_reason);

    // System initialization (including LDO, LCD, touch, LED and all hardware)
    system_init();
    MAIN_INFO("System initialized successfully");

    // Initialize UI components while holding the LVGL port mutex.
    lvgl_port_lock(0);
    ui_init();
    WaveHardwareReady(wave_hardware_ready);
    lvgl_port_unlock();
    MAIN_INFO("UI initialized successfully");

    // Initialize encoder only after the first UI frame exists. Encoder
    // failure must never prevent ScopeBuddy from showing its interface.
    MAIN_INFO("Initializing rotary encoder...");
    esp_err_t err = encoder_input_init();
    if (err == ESP_OK) {
        MAIN_INFO("Rotary encoder initialized successfully");
    } else {
        MAIN_ERROR("Encoder unavailable: %s", esp_err_to_name(err));
    }
}
