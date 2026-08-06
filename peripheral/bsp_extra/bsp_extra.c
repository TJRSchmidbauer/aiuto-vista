/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_extra.h"
#include "driver/rmt_tx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/* Wave output: GPIO26 (audio in-) for channel 1, GPIO27 (CN1) for channel 2.
 * Both pins are driven as push-pull outputs on the ESP32. */
#define WAVE_GPIO              GPIO_NUM_26
#define WAVE_GPIO_SECONDARY    GPIO_NUM_27
#define WAVE_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define WAVE_LEDC_TIMER        LEDC_TIMER_0
#define WAVE_LEDC_CHANNEL      LEDC_CHANNEL_0
#define WAVE_DUTY_RESOLUTION   LEDC_TIMER_10_BIT
#define WAVE_DUTY_MAX          ((1U << 10) - 1U)
#define SEQUENCE_RMT_RESOLUTION_HZ 1000000U
#define SEQUENCE_RMT_SYMBOLS       48U
#define SEQUENCE_RMT_MAX_DURATION  32767U
typedef struct {
    gpio_num_t gpio;
    rmt_channel_handle_t channel;
    rmt_encoder_handle_t encoder;
    rmt_symbol_word_t symbols[SEQUENCE_RMT_SYMBOLS];
    size_t symbol_count;
    bool enabled;
} sequence_output_t;

static uint8_t wave_duty_percent = 50;
static sequence_output_t sequence_outputs[] = {
    { .gpio = WAVE_GPIO },
    { .gpio = WAVE_GPIO_SECONDARY },
};
static rmt_sync_manager_handle_t sequence_sync_manager;

esp_err_t gpio_extra_init()                    // Function to initialize the wave GPIO as output
{
    const gpio_config_t gpio_cofig = {         // Define GPIO configuration structure
        .pin_bit_mask = (1ULL << WAVE_GPIO),   // Select the wave GPIO by setting its bit in the mask
        .mode = GPIO_MODE_OUTPUT,              // Configure the wave GPIO as output mode
        .pull_up_en = false,                   // Disable internal pull-up resistor
        .pull_down_en = false,                 // Disable internal pull-down resistor
        .intr_type = GPIO_INTR_DISABLE,        // Disable GPIO interrupt for this pin
    };
    return gpio_config(&gpio_cofig);           // Apply the configuration and report any failure
}

esp_err_t gpio_extra_set_level(bool level)     // Function to set output level of the wave GPIO
{
    return gpio_set_level(WAVE_GPIO, level);   // Set the wave GPIO level and report any failure
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
         * with the APB clock and 10-bit resolution. A fixed source avoids
         * runtime clock-source changes while hard challenges are switching. */
        .clk_cfg = LEDC_AUTO_CLK,
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
    ESP_RETURN_ON_ERROR(gpio_sequence_stop(), EXTRA_TAG, "Stopping RMT output failed");
    ESP_RETURN_ON_ERROR(ledc_set_pin(WAVE_GPIO, WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL),
                        EXTRA_TAG, "Routing LEDC output failed");
    ESP_RETURN_ON_ERROR(ledc_set_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL,
                                      (WAVE_DUTY_MAX * wave_duty_percent) / 100U),
                        EXTRA_TAG, "Setting wave duty failed");
    return ledc_update_duty(WAVE_LEDC_MODE, WAVE_LEDC_CHANNEL);
}

esp_err_t gpio_wave_stop(void)
{
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

static esp_err_t ensure_sequence_output(sequence_output_t *output)
{
    if (output->channel && output->encoder) return ESP_OK;

    rmt_tx_channel_config_t channel_config = {
        .gpio_num = output->gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = SEQUENCE_RMT_RESOLUTION_HZ,
        .mem_block_symbols = SEQUENCE_RMT_SYMBOLS,
        .trans_queue_depth = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &output->channel),
                        EXTRA_TAG, "Creating RMT channel failed");

    const rmt_copy_encoder_config_t encoder_config;
    esp_err_t err = rmt_new_copy_encoder(&encoder_config, &output->encoder);
    if (err != ESP_OK) {
        rmt_del_channel(output->channel);
        output->channel = NULL;
    }
    return err;
}

static esp_err_t drive_output_low(sequence_output_t *output)
{
    ESP_RETURN_ON_ERROR(gpio_set_direction(output->gpio, GPIO_MODE_OUTPUT),
                        EXTRA_TAG, "Setting stopped output direction failed");
    return gpio_set_level(output->gpio, 0);
}

static esp_err_t compile_sequence(const gpio_wave_segment_t *segments, size_t segment_count,
                                  rmt_symbol_word_t *symbols, size_t *symbol_count,
                                  uint64_t *total_duration_us)
{
    if (segments == NULL || segment_count == 0 || symbols == NULL ||
        symbol_count == NULL || total_duration_us == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bool phase_levels[SEQUENCE_RMT_SYMBOLS * 2U];
    uint16_t phase_durations[SEQUENCE_RMT_SYMBOLS * 2U];
    size_t phase_count = 0;
    uint64_t total_duration = 0;
    size_t segment = 0;
    while (segment < segment_count) {
        bool level = segments[segment].level;
        uint64_t run_duration = 0;
        do {
            if (segments[segment].duration_us == 0) return ESP_ERR_INVALID_ARG;
            run_duration += segments[segment].duration_us;
            total_duration += segments[segment].duration_us;
            ++segment;
        } while (segment < segment_count && segments[segment].level == level);

        while (run_duration > 0) {
            if (phase_count >= SEQUENCE_RMT_SYMBOLS * 2U) return ESP_ERR_INVALID_SIZE;
            uint32_t duration = run_duration > SEQUENCE_RMT_MAX_DURATION ?
                                SEQUENCE_RMT_MAX_DURATION : (uint32_t)run_duration;
            phase_levels[phase_count] = level;
            phase_durations[phase_count] = (uint16_t)duration;
            ++phase_count;
            run_duration -= duration;
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

    *symbol_count = phase_count / 2U;
    *total_duration_us = total_duration;
    for (size_t symbol = 0; symbol < *symbol_count; ++symbol) {
        symbols[symbol] = (rmt_symbol_word_t) {
            .level0 = phase_levels[symbol * 2U],
            .duration0 = phase_durations[symbol * 2U],
            .level1 = phase_levels[symbol * 2U + 1U],
            .duration1 = phase_durations[symbol * 2U + 1U],
        };
    }
    return ESP_OK;
}

esp_err_t gpio_sequence_pair_stop(void)
{
    esp_err_t first_error = ESP_OK;
    if (sequence_sync_manager) {
        esp_err_t err = rmt_del_sync_manager(sequence_sync_manager);
        if (err != ESP_OK) first_error = err;
        else sequence_sync_manager = NULL;
    }
    for (size_t index = 0; index < 2; ++index) {
        sequence_output_t *output = &sequence_outputs[index];
        if (!output->enabled) continue;
        esp_err_t err = rmt_disable(output->channel);
        if (err != ESP_OK && first_error == ESP_OK) first_error = err;
        if (err == ESP_OK) {
            output->enabled = false;
            err = drive_output_low(output);
            if (err != ESP_OK && first_error == ESP_OK) first_error = err;
        }
    }
    return first_error;
}

esp_err_t gpio_sequence_stop(void)
{
    return gpio_sequence_pair_stop();
}

esp_err_t gpio_sequence_start(const gpio_wave_segment_t *segments,
                              size_t segment_count, bool loop)
{
    rmt_symbol_word_t compiled[SEQUENCE_RMT_SYMBOLS];
    size_t symbol_count;
    uint64_t total_duration;
    ESP_RETURN_ON_ERROR(compile_sequence(segments, segment_count, compiled,
                                         &symbol_count, &total_duration),
                        EXTRA_TAG, "Compiling sequence failed");
    (void)total_duration;

    ESP_RETURN_ON_ERROR(gpio_sequence_pair_stop(), EXTRA_TAG, "Stopping previous RMT output failed");
    ESP_RETURN_ON_ERROR(gpio_wave_stop(), EXTRA_TAG, "Stopping LEDC output failed");
    sequence_output_t *output = &sequence_outputs[0];
    ESP_RETURN_ON_ERROR(ensure_sequence_output(output), EXTRA_TAG, "Initializing RMT output failed");
    memcpy(output->symbols, compiled, symbol_count * sizeof(compiled[0]));
    output->symbol_count = symbol_count;
    ESP_RETURN_ON_ERROR(rmt_tx_switch_gpio(output->channel, output->gpio, false),
                        EXTRA_TAG, "Routing sequence RMT output failed");
    ESP_RETURN_ON_ERROR(rmt_enable(output->channel), EXTRA_TAG, "Enabling sequence RMT failed");
    output->enabled = true;

    const rmt_transmit_config_t transmit_config = {
        .loop_count = loop ? -1 : 0,
        .flags.eot_level = 0,
    };
    esp_err_t err = rmt_transmit(output->channel, output->encoder, output->symbols,
                                 output->symbol_count * sizeof(output->symbols[0]),
                                 &transmit_config);
    if (err != ESP_OK) gpio_sequence_pair_stop();
    return err;
}

esp_err_t gpio_sequence_pair_start(const gpio_wave_segment_t *channel_1_segments,
                                   size_t channel_1_segment_count,
                                   const gpio_wave_segment_t *channel_2_segments,
                                   size_t channel_2_segment_count,
                                   bool loop)
{
    rmt_symbol_word_t compiled[2][SEQUENCE_RMT_SYMBOLS];
    size_t symbol_counts[2];
    uint64_t total_durations[2];
    ESP_RETURN_ON_ERROR(compile_sequence(channel_1_segments, channel_1_segment_count,
                                         compiled[0], &symbol_counts[0], &total_durations[0]),
                        EXTRA_TAG, "Compiling channel 1 sequence failed");
    ESP_RETURN_ON_ERROR(compile_sequence(channel_2_segments, channel_2_segment_count,
                                         compiled[1], &symbol_counts[1], &total_durations[1]),
                        EXTRA_TAG, "Compiling channel 2 sequence failed");
    if (total_durations[0] != total_durations[1]) {
        ESP_LOGE(EXTRA_TAG, "Channel duration mismatch: %llu us versus %llu us",
                 (unsigned long long)total_durations[0],
                 (unsigned long long)total_durations[1]);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_RETURN_ON_ERROR(gpio_sequence_pair_stop(), EXTRA_TAG, "Stopping previous RMT output failed");
    ESP_RETURN_ON_ERROR(gpio_wave_stop(), EXTRA_TAG, "Stopping LEDC output failed");
    for (size_t index = 0; index < 2; ++index) {
        sequence_output_t *output = &sequence_outputs[index];
        esp_err_t err = ensure_sequence_output(output);
        if (err != ESP_OK) {
            gpio_sequence_pair_stop();
            return err;
        }
        memcpy(output->symbols, compiled[index], symbol_counts[index] * sizeof(compiled[index][0]));
        output->symbol_count = symbol_counts[index];
        err = rmt_tx_switch_gpio(output->channel, output->gpio, false);
        if (err == ESP_OK) err = rmt_enable(output->channel);
        if (err != ESP_OK) {
            gpio_sequence_pair_stop();
            return err;
        }
        output->enabled = true;
    }

    rmt_channel_handle_t channels[] = {
        sequence_outputs[0].channel,
        sequence_outputs[1].channel,
    };
    const rmt_sync_manager_config_t manager_config = {
        .tx_channel_array = channels,
        .array_size = 2,
    };
    esp_err_t err = rmt_new_sync_manager(&manager_config, &sequence_sync_manager);
    if (err != ESP_OK) {
        gpio_sequence_pair_stop();
        return err;
    }

    const rmt_transmit_config_t transmit_config = {
        .loop_count = loop ? -1 : 0,
        .flags.eot_level = 0,
    };
    for (size_t index = 0; index < 2; ++index) {
        sequence_output_t *output = &sequence_outputs[index];
        err = rmt_transmit(output->channel, output->encoder, output->symbols,
                           output->symbol_count * sizeof(output->symbols[0]),
                           &transmit_config);
        if (err != ESP_OK) {
            gpio_sequence_pair_stop();
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t gpio_sync_test_start(void)
{
    const gpio_wave_segment_t channel_1[] = {
        { .level = true, .duration_us = 500 },
        { .level = false, .duration_us = 500 },
    };
    const gpio_wave_segment_t channel_2[] = {
        { .level = false, .duration_us = 100 },
        { .level = true, .duration_us = 500 },
        { .level = false, .duration_us = 400 },
    };
    return gpio_sequence_pair_start(channel_1, 2, channel_2, 3, true);
}

esp_err_t gpio_sync_test_stop(void)
{
    return gpio_sequence_pair_stop();
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
