/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_extra.h"
#include "driver/rmt_tx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

#define WAVE_GPIO              GPIO_NUM_48
#define WAVE_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define WAVE_LEDC_TIMER        LEDC_TIMER_0
#define WAVE_LEDC_CHANNEL      LEDC_CHANNEL_0
#define WAVE_DUTY_RESOLUTION   LEDC_TIMER_10_BIT
#define WAVE_DUTY_MAX          ((1U << 10) - 1U)
#define BURST_RMT_RESOLUTION_HZ 1000000U
#define BURST_RMT_SYMBOLS       48U
#define BURST_RMT_MAX_DURATION  32767U

static uint8_t wave_duty_percent = 50;
static rmt_channel_handle_t burst_channel;
static rmt_encoder_handle_t burst_encoder;
static rmt_symbol_word_t burst_symbols[BURST_RMT_SYMBOLS];
static bool burst_channel_enabled;

esp_err_t gpio_extra_init()                    // Function to initialize GPIO48 as output
{
    const gpio_config_t gpio_cofig = {         // Define GPIO configuration structure
        .pin_bit_mask = (1ULL << 48),          // Select GPIO48 by setting bit 48 in the mask
        .mode = GPIO_MODE_OUTPUT,              // Configure GPIO48 as output mode
        .pull_up_en = false,                   // Disable internal pull-up resistor
        .pull_down_en = false,                 // Disable internal pull-down resistor
        .intr_type = GPIO_INTR_DISABLE,        // Disable GPIO interrupt for this pin
    };
    return gpio_config(&gpio_cofig);           // Apply the configuration and report any failure
}

esp_err_t gpio_extra_set_level(bool level)     // Function to set output level of GPIO48
{
    return gpio_set_level(48, level);          // Set GPIO48 and report any failure
}

esp_err_t gpio_wave_init(uint32_t frequency_hz, uint8_t duty_percent)
{
    if (frequency_hz < 10 || frequency_hz > 20000 || duty_percent < 1 || duty_percent > 99) {
        return ESP_ERR_INVALID_ARG;
    }

    ledc_timer_config_t timer = {
        .speed_mode = WAVE_LEDC_MODE,
        .duty_resolution = WAVE_DUTY_RESOLUTION,
        .timer_num = WAVE_LEDC_TIMER,
        .freq_hz = frequency_hz,
        /* All supported output frequencies (10 Hz to 20 kHz) fit comfortably
         * with the 40 MHz crystal and 10-bit resolution. A fixed source avoids
         * runtime clock-source changes while hard challenges are switching. */
        .clk_cfg = LEDC_USE_XTAL_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), EXTRA_TAG, "LEDC timer setup failed");

    wave_duty_percent = duty_percent;
    ledc_channel_config_t channel = {
        .gpio_num = WAVE_GPIO,
        .speed_mode = WAVE_LEDC_MODE,
        .channel = WAVE_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = WAVE_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&channel);
}

esp_err_t gpio_wave_start(void)
{
    ESP_RETURN_ON_ERROR(ledc_set_pin(WAVE_GPIO, WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL),
                        EXTRA_TAG, "Routing LEDC output failed");
    ESP_RETURN_ON_ERROR(ledc_set_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL,
                                      (WAVE_DUTY_MAX * wave_duty_percent) / 100U),
                        EXTRA_TAG, "Setting wave duty failed");
    return ledc_update_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL);
}

esp_err_t gpio_wave_stop(void)
{
    ESP_RETURN_ON_ERROR(ledc_set_pin(WAVE_GPIO, WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL),
                        EXTRA_TAG, "Routing LEDC output failed");
    return ledc_stop(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL, 0);
}

esp_err_t gpio_wave_set_frequency(uint32_t frequency_hz)
{
    if (frequency_hz < 10 || frequency_hz > 20000) {
        return ESP_ERR_INVALID_ARG;
    }
    return ledc_set_freq(WAVE_LEDC_MODE, WAVE_LEDC_TIMER, frequency_hz);
}

esp_err_t gpio_wave_set_duty(uint8_t duty_percent)
{
    if (duty_percent < 1 || duty_percent > 99) {
        return ESP_ERR_INVALID_ARG;
    }
    wave_duty_percent = duty_percent;
    ESP_RETURN_ON_ERROR(ledc_set_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL,
                                      (WAVE_DUTY_MAX * wave_duty_percent) / 100U),
                        EXTRA_TAG, "Setting wave duty failed");
    return ledc_update_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL);
}

esp_err_t gpio_burst_get_timing_us(uint32_t frequency_hz, uint8_t duty_percent,
                                   uint32_t *period_us, uint32_t *high_time_us)
{
    if (frequency_hz < 100 || frequency_hz > 2000 ||
        duty_percent < 1 || duty_percent > 99 ||
        period_us == NULL || high_time_us == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t period = (BURST_RMT_RESOLUTION_HZ + frequency_hz / 2U) / frequency_hz;
    uint32_t high_time = (period * duty_percent + 50U) / 100U;
    if (high_time == 0) high_time = 1;
    if (high_time >= period) high_time = period - 1U;
    *period_us = period;
    *high_time_us = high_time;
    return ESP_OK;
}

static esp_err_t ensure_burst_rmt(void)
{
    if (burst_channel && burst_encoder) return ESP_OK;

    rmt_tx_channel_config_t channel_config = {
        .gpio_num = WAVE_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = BURST_RMT_RESOLUTION_HZ,
        .mem_block_symbols = BURST_RMT_SYMBOLS,
        .trans_queue_depth = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &burst_channel),
                        EXTRA_TAG, "Creating burst RMT channel failed");

    const rmt_copy_encoder_config_t encoder_config;
    esp_err_t err = rmt_new_copy_encoder(&encoder_config, &burst_encoder);
    if (err != ESP_OK) {
        rmt_del_channel(burst_channel);
        burst_channel = NULL;
    }
    return err;
}

esp_err_t gpio_burst_stop(void)
{
    if (!burst_channel_enabled) return ESP_OK;
    esp_err_t err = rmt_disable(burst_channel);
    if (err == ESP_OK) burst_channel_enabled = false;
    return err;
}

esp_err_t gpio_burst_start(uint32_t frequency_hz, uint8_t duty_percent,
                           uint8_t pulse_count, uint16_t pause_ms)
{
    if (pulse_count == 0 || pulse_count > 15 || pause_ms < 10 || pause_ms > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t period_us;
    uint32_t high_time_us;
    ESP_RETURN_ON_ERROR(gpio_burst_get_timing_us(frequency_hz, duty_percent,
                                                 &period_us, &high_time_us),
                        EXTRA_TAG, "Calculating burst timing failed");
    ESP_RETURN_ON_ERROR(ensure_burst_rmt(), EXTRA_TAG, "Initializing burst RMT failed");
    ESP_RETURN_ON_ERROR(gpio_burst_stop(), EXTRA_TAG, "Stopping previous burst failed");

    size_t symbol_count = 0;
    uint32_t low_time_us = period_us - high_time_us;
    for (uint8_t pulse = 1; pulse < pulse_count; ++pulse) {
        burst_symbols[symbol_count++] = (rmt_symbol_word_t) {
            .level0 = 1,
            .duration0 = high_time_us,
            .level1 = 0,
            .duration1 = low_time_us,
        };
    }

    /* Use an odd number of low segments. Together with the last HIGH phase
     * this always fills complete RMT symbols while preserving the exact pause
     * from the final falling edge to the next burst's first rising edge. */
    uint32_t pause_us = (uint32_t)pause_ms * 1000U;
    uint32_t pause_segments = (pause_us + BURST_RMT_MAX_DURATION - 1U) /
                              BURST_RMT_MAX_DURATION;
    if ((pause_segments & 1U) == 0U) ++pause_segments;
    uint32_t pause_base = pause_us / pause_segments;
    uint32_t pause_remainder = pause_us % pause_segments;
    uint32_t pause_duration = pause_base;
    if (pause_remainder > 0U) {
        ++pause_duration;
        --pause_remainder;
    }
    burst_symbols[symbol_count] = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = high_time_us,
        .level1 = 0,
        .duration1 = pause_duration,
    };
    ++symbol_count;

    for (uint32_t segment = 1; segment < pause_segments; segment += 2U) {
        uint32_t duration0 = pause_base;
        uint32_t duration1 = pause_base;
        if (pause_remainder > 0U) {
            ++duration0;
            --pause_remainder;
        }
        if (pause_remainder > 0U) {
            ++duration1;
            --pause_remainder;
        }
        if (symbol_count >= BURST_RMT_SYMBOLS) return ESP_ERR_INVALID_SIZE;
        burst_symbols[symbol_count++] = (rmt_symbol_word_t) {
            .level0 = 0,
            .duration0 = duration0,
            .level1 = 0,
            .duration1 = duration1,
        };
    }

    ESP_RETURN_ON_ERROR(rmt_tx_switch_gpio(burst_channel, WAVE_GPIO, false),
                        EXTRA_TAG, "Routing burst RMT output failed");
    ESP_RETURN_ON_ERROR(rmt_enable(burst_channel), EXTRA_TAG, "Enabling burst RMT failed");
    burst_channel_enabled = true;

    const rmt_transmit_config_t transmit_config = {
        .loop_count = -1,
        .flags.eot_level = 0,
    };
    esp_err_t err = rmt_transmit(burst_channel, burst_encoder, burst_symbols,
                                 symbol_count * sizeof(burst_symbols[0]), &transmit_config);
    if (err != ESP_OK) {
        rmt_disable(burst_channel);
        burst_channel_enabled = false;
    }
    return err;
}
