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
#define SEQUENCE_RMT_RESOLUTION_HZ 1000000U
#define SEQUENCE_RMT_SYMBOLS       48U
#define SEQUENCE_RMT_MAX_DURATION  32767U

static uint8_t wave_duty_percent = 50;
static rmt_channel_handle_t sequence_channel;
static rmt_encoder_handle_t sequence_encoder;
static rmt_symbol_word_t sequence_symbols[SEQUENCE_RMT_SYMBOLS];
static bool sequence_channel_enabled;

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

esp_err_t gpio_wave_get_effective(uint32_t *frequency_hz, uint8_t *duty_percent)
{
    if (frequency_hz == NULL || duty_percent == NULL) return ESP_ERR_INVALID_ARG;
    uint32_t frequency = ledc_get_freq(WAVE_LEDC_MODE, WAVE_LEDC_TIMER);
    if (frequency == 0) return ESP_ERR_INVALID_STATE;
    uint32_t duty = ledc_get_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL);
    *frequency_hz = frequency;
    *duty_percent = (uint8_t)((duty * 100U + WAVE_DUTY_MAX / 2U) / WAVE_DUTY_MAX);
    return ESP_OK;
}

esp_err_t gpio_burst_get_timing_us(uint32_t frequency_hz, uint8_t duty_percent,
                                   uint32_t *period_us, uint32_t *high_time_us)
{
    if (frequency_hz < 100 || frequency_hz > 2000 ||
        duty_percent < 1 || duty_percent > 99 ||
        period_us == NULL || high_time_us == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t period = (SEQUENCE_RMT_RESOLUTION_HZ + frequency_hz / 2U) / frequency_hz;
    uint32_t high_time = (period * duty_percent + 50U) / 100U;
    if (high_time == 0) high_time = 1;
    if (high_time >= period) high_time = period - 1U;
    *period_us = period;
    *high_time_us = high_time;
    return ESP_OK;
}

static esp_err_t ensure_sequence_rmt(void)
{
    if (sequence_channel && sequence_encoder) return ESP_OK;

    rmt_tx_channel_config_t channel_config = {
        .gpio_num = WAVE_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = SEQUENCE_RMT_RESOLUTION_HZ,
        .mem_block_symbols = SEQUENCE_RMT_SYMBOLS,
        .trans_queue_depth = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &sequence_channel),
                        EXTRA_TAG, "Creating sequence RMT channel failed");

    const rmt_copy_encoder_config_t encoder_config;
    esp_err_t err = rmt_new_copy_encoder(&encoder_config, &sequence_encoder);
    if (err != ESP_OK) {
        rmt_del_channel(sequence_channel);
        sequence_channel = NULL;
    }
    return err;
}

esp_err_t gpio_sequence_stop(void)
{
    if (!sequence_channel_enabled) return ESP_OK;
    esp_err_t err = rmt_disable(sequence_channel);
    if (err == ESP_OK) sequence_channel_enabled = false;
    return err;
}

esp_err_t gpio_sequence_start(const gpio_wave_segment_t *segments,
                              size_t segment_count, bool loop)
{
    if (segments == NULL || segment_count == 0) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ensure_sequence_rmt(), EXTRA_TAG, "Initializing sequence RMT failed");
    ESP_RETURN_ON_ERROR(gpio_sequence_stop(), EXTRA_TAG, "Stopping previous sequence failed");

    bool phase_levels[SEQUENCE_RMT_SYMBOLS * 2U];
    uint16_t phase_durations[SEQUENCE_RMT_SYMBOLS * 2U];
    size_t phase_count = 0;
    for (size_t segment = 0; segment < segment_count; ++segment) {
        uint32_t remaining = segments[segment].duration_us;
        if (remaining == 0) return ESP_ERR_INVALID_ARG;
        while (remaining > 0) {
            if (phase_count >= SEQUENCE_RMT_SYMBOLS * 2U) return ESP_ERR_INVALID_SIZE;
            uint32_t duration = remaining > SEQUENCE_RMT_MAX_DURATION ?
                                SEQUENCE_RMT_MAX_DURATION : remaining;
            phase_levels[phase_count] = segments[segment].level;
            phase_durations[phase_count] = (uint16_t)duration;
            ++phase_count;
            remaining -= duration;
        }
    }

    if ((phase_count & 1U) != 0U) {
        if (phase_count >= SEQUENCE_RMT_SYMBOLS * 2U || phase_durations[phase_count - 1U] < 2U) {
            return ESP_ERR_INVALID_SIZE;
        }
        uint16_t original = phase_durations[phase_count - 1U];
        phase_durations[phase_count - 1U] = original / 2U;
        phase_levels[phase_count] = phase_levels[phase_count - 1U];
        phase_durations[phase_count] = original - phase_durations[phase_count - 1U];
        ++phase_count;
    }

    size_t symbol_count = phase_count / 2U;
    for (size_t symbol = 0; symbol < symbol_count; ++symbol) {
        sequence_symbols[symbol] = (rmt_symbol_word_t) {
            .level0 = phase_levels[symbol * 2U],
            .duration0 = phase_durations[symbol * 2U],
            .level1 = phase_levels[symbol * 2U + 1U],
            .duration1 = phase_durations[symbol * 2U + 1U],
        };
    }

    ESP_RETURN_ON_ERROR(rmt_tx_switch_gpio(sequence_channel, WAVE_GPIO, false),
                        EXTRA_TAG, "Routing sequence RMT output failed");
    ESP_RETURN_ON_ERROR(rmt_enable(sequence_channel), EXTRA_TAG, "Enabling sequence RMT failed");
    sequence_channel_enabled = true;
    const rmt_transmit_config_t transmit_config = {
        .loop_count = loop ? -1 : 0,
        .flags.eot_level = 0,
    };
    esp_err_t err = rmt_transmit(sequence_channel, sequence_encoder, sequence_symbols,
                                 symbol_count * sizeof(sequence_symbols[0]), &transmit_config);
    if (err != ESP_OK) {
        rmt_disable(sequence_channel);
        sequence_channel_enabled = false;
    }
    return err;
}

esp_err_t gpio_burst_stop(void)
{
    return gpio_sequence_stop();
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
    gpio_wave_segment_t segments[32];
    size_t segment_count = 0;
    uint32_t low_time_us = period_us - high_time_us;
    for (uint8_t pulse = 0; pulse < pulse_count; ++pulse) {
        segments[segment_count++] = (gpio_wave_segment_t){ .level = true, .duration_us = high_time_us };
        if (pulse + 1U < pulse_count) {
            segments[segment_count++] = (gpio_wave_segment_t){ .level = false, .duration_us = low_time_us };
        }
    }
    segments[segment_count++] = (gpio_wave_segment_t) {
        .level = false,
        .duration_us = (uint32_t)pause_ms * 1000U,
    };
    return gpio_sequence_start(segments, segment_count, true);
}
