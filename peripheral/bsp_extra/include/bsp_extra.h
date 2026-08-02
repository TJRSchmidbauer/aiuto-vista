#ifndef _BSP_EXTRA_H_
#define _BSP_EXTRA_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

#include <string.h>      // Standard C library for string handling functions
#include <stdint.h>      // Standard C library for fixed-width integer types
#include "esp_log.h"     // ESP-IDF logging library for debug/info/error logs
#include "esp_err.h"     // ESP-IDF error code definitions and handling utilities
#include "esp_check.h"
#include "driver/gpio.h" // ESP-IDF GPIO driver for configuring and controlling pins
#include "driver/ledc.h"
#include <stddef.h>

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

#define EXTRA_TAG "EXTRA"                           // Define log tag name "EXTRA" used for identifying log messages
#define EXTRA_INFO(fmt, ...) ESP_LOGI(EXTRA_TAG, fmt, ##__VA_ARGS__)   // Macro for info-level logging with tag "EXTRA"
#define EXTRA_DEBUG(fmt, ...) ESP_LOGD(EXTRA_TAG, fmt, ##__VA_ARGS__)  // Macro for debug-level logging with tag "EXTRA"
#define EXTRA_ERROR(fmt, ...) ESP_LOGE(EXTRA_TAG, fmt, ##__VA_ARGS__)  // Macro for error-level logging with tag "EXTRA"

esp_err_t gpio_extra_init();                        // Function declaration for initializing GPIO
esp_err_t gpio_extra_set_level(bool level);         // Function declaration for setting GPIO output level (high/low)
esp_err_t gpio_wave_init(uint32_t frequency_hz, uint8_t duty_percent);
esp_err_t gpio_wave_start(void);
esp_err_t gpio_wave_stop(void);
esp_err_t gpio_wave_set_frequency(uint32_t frequency_hz);
esp_err_t gpio_wave_set_duty(uint8_t duty_percent);
esp_err_t gpio_wave_get_effective(uint32_t *frequency_hz, uint8_t *duty_percent);

typedef struct {
    bool level;
    uint32_t duration_us;
} gpio_wave_segment_t;

esp_err_t gpio_sequence_start(const gpio_wave_segment_t *segments,
                              size_t segment_count, bool loop);
esp_err_t gpio_sequence_stop(void);
esp_err_t gpio_burst_start(uint32_t frequency_hz, uint8_t duty_percent,
                           uint8_t pulse_count, uint16_t pause_ms);
esp_err_t gpio_burst_stop(void);
esp_err_t gpio_burst_get_timing_us(uint32_t frequency_hz, uint8_t duty_percent,
                                   uint32_t *period_us, uint32_t *high_time_us);

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
