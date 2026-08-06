// main.c — Aiuto-Vista firmware entry point.
// Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
// Target hardware: Cheap Yellow Display ESP32-2432S028 (ESP32-WROOM-32).
#include "main.h"
#include "ui.h"
#include "esp_system.h"
#include "nvs_flash.h"

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

    // 1. Initialize touch panel (low-level driver)
    MAIN_INFO("Initializing touch panel...");
    err = touch_init();
    if (err != ESP_OK) init_fail_handler("Touch", err);
    MAIN_INFO("Touch panel init success");

    // 2. Initialize LCD hardware and LVGL (must initialize before turning on backlight)
    err = display_init();
    if (err != ESP_OK) init_fail_handler("LCD", err);
    MAIN_INFO("LCD init success");

    // 3. Turn on LCD backlight (brightness set to 100 = max)
    err = set_lcd_blight(100);
    if (err != ESP_OK) init_fail_handler("LCD Backlight", err);
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    // 4. Initialize the waveform generator on GPIO26, initially stopped
    MAIN_INFO("Initializing GPIO26 waveform generator...");
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

    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MAIN_INFO("Reinitializing NVS settings storage...");
        nvs_err = nvs_flash_erase();
        if (nvs_err == ESP_OK) nvs_err = nvs_flash_init();
    }
    if (nvs_err != ESP_OK) {
        MAIN_ERROR("NVS unavailable, settings will use defaults: %s", esp_err_to_name(nvs_err));
    }

    // System initialization (including LDO, LCD, touch, LED and all hardware)
    system_init();
    MAIN_INFO("System initialized successfully");

    // Initialize UI components while holding the LVGL port mutex.
    lvgl_port_lock(0);
    ui_init();
    WaveHardwareReady(wave_hardware_ready);
    lvgl_port_unlock();
    MAIN_INFO("UI initialized successfully");
}
