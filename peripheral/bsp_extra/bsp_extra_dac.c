// DAC waveform backend for the MCP4725 variant.
// Replaces bsp_extra.c when CONFIG_CYD_MCP4725_SUPPORT is enabled.
// The public interface (bsp_extra.h) is identical to the GPIO/LEDC/RMT
// backend, so lessons, measurements and the UI run unchanged.
//
// Signals are produced by two MCP4725 12-bit DACs on the I2C bus:
//   channel 1 = DAC A (CONFIG_CYD_DAC_DAC_A_ADDRESS, default 0x60)
//   channel 2 = DAC B (CONFIG_CYD_DAC_DAC_B_ADDRESS, default 0x61)
// LOW equals 0 V, HIGH equals 3.3 V (full scale). The tachometer exercise
// renders a sine (reality of a magnetic pickup) as analog voltage levels.
//
// Timing: a renderer task drives the edges with microsecond accuracy. An
// esp_timer fires shortly before each edge deadline; the task then writes
// the new level and re-arms the timer, so the CPU is mostly idle and the
// watchdog is never starved. Write durations are measured and fed back so
// each level transition lands on its deadline.
// Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
#include "bsp_extra.h"

#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

#define DAC_MAX_SEGMENTS   64U
#define DAC_LOW_VALUE      0x0000U
#define DAC_HIGH_VALUE     0x0FFFU
#define DAC_SINE_MID       2048U
#define DAC_SINE_AMPLITUDE 1950U
#define DAC_MIN_SEGMENT_US 100U
#define DAC_SINE_TICK_US   100U
#define DAC_SINE_MAX_HZ    500U
#define DAC_WRITE_EST_INIT 60U
#define DAC_WRITE_MARGIN   3U
#define DAC_POLL_LEAD      120U
#define DAC_HEARTBEAT_US   50000U

typedef enum {
    DAC_MODE_IDLE,
    DAC_MODE_SQUARE,
    DAC_MODE_SINE,
    DAC_MODE_SEQUENCE,
} dac_mode_t;

typedef struct {
    gpio_wave_segment_t segments[DAC_MAX_SEGMENTS];
    size_t segment_count;
    size_t index;
    bool loop;
    bool active;
    bool level;
    uint32_t deadline_us;
} dac_channel_t;

static i2c_master_bus_handle_t dac_bus;
static i2c_master_dev_handle_t dac_devices[2];
static volatile dac_mode_t dac_mode = DAC_MODE_IDLE;
static volatile uint32_t dac_frequency_hz;
static volatile uint8_t dac_duty_percent;
static volatile bool dac_sine_style;
static dac_channel_t dac_channels[2];
static TaskHandle_t dac_renderer_task;
static esp_timer_handle_t dac_edge_timer;

static uint32_t dac_now_us(void)
{
    return (uint32_t)esp_timer_get_time();
}

static void dac_write_raw(uint32_t index, uint16_t value)
{
    const uint8_t frame[2] = { (uint8_t)(value >> 4U), (uint8_t)((value & 0x0FU) << 4U) };
    i2c_master_transmit(dac_devices[index], frame, sizeof(frame), 2000);
}

static void dac_write_level(uint32_t index, bool level)
{
    dac_write_raw(index, level ? DAC_HIGH_VALUE : DAC_LOW_VALUE);
}

static void dac_set_both_levels(bool level)
{
    dac_write_level(0, level);
    dac_write_level(1, level);
}

static void dac_edge_timer_callback(void *argument)
{
    (void)argument;
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(dac_renderer_task, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void dac_arm_timer(uint32_t delay_us)
{
    if (delay_us == 0) delay_us = 1;
    esp_timer_stop(dac_edge_timer);
    esp_timer_start_once(dac_edge_timer, delay_us);
}

static void dac_wait_for_wake(void)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static bool dac_advance_sequence_channel(uint32_t index)
{
    dac_channel_t *channel = &dac_channels[index];
    if (channel->segment_count == 0) {
        return false;
    }
    if (channel->index + 1U >= channel->segment_count) {
        if (!channel->loop) return false;
        channel->index = 0;
    } else {
        channel->index += 1U;
    }
    const gpio_wave_segment_t *segment = &channel->segments[channel->index];
    channel->level = segment->level;
    channel->deadline_us += segment->duration_us < DAC_MIN_SEGMENT_US
                                ? DAC_MIN_SEGMENT_US : segment->duration_us;
    return true;
}

static void dac_renderer_fn(void *argument)
{
    (void)argument;
    uint32_t write_estimate = DAC_WRITE_EST_INIT;
    uint32_t last_error_log_us = 0;

    for (;;) {
        const uint32_t now = dac_now_us();
        const dac_mode_t mode = dac_mode;
        uint32_t earliest = 0;
        uint32_t work_us = DAC_WRITE_EST_INIT;

        if (mode == DAC_MODE_SQUARE || mode == DAC_MODE_SINE) {
            earliest = dac_channels[0].deadline_us;
            work_us = (mode == DAC_MODE_SINE) ? DAC_WRITE_EST_INIT : write_estimate;
        } else if (mode == DAC_MODE_SEQUENCE) {
            for (uint32_t index = 0; index < 2; ++index) {
                if (!dac_channels[index].active) continue;
                if (earliest == 0 || dac_channels[index].deadline_us < earliest) {
                    earliest = dac_channels[index].deadline_us;
                }
            }
            work_us = write_estimate;
        }

        if (earliest == 0) {
            dac_arm_timer(DAC_HEARTBEAT_US);
            dac_wait_for_wake();
            continue;
        }

        const int32_t fire_delta = (int32_t)(earliest - work_us - DAC_WRITE_MARGIN - DAC_POLL_LEAD - now);
        if (fire_delta > 0) {
            dac_arm_timer((uint32_t)fire_delta);
        }
        dac_wait_for_wake();

        const uint32_t write_start = dac_now_us();
        if (mode == DAC_MODE_SINE) {
            const uint32_t frequency = dac_frequency_hz;
            const uint32_t tick = dac_channels[0].deadline_us;
            const double phase = 2.0 * M_PI * (double)frequency * (double)tick / 1000000.0;
            const int32_t value = DAC_SINE_MID + (int32_t)(DAC_SINE_AMPLITUDE * sin(phase));
            dac_write_raw(0, (uint16_t)(value < 0 ? 0 : value > 4095 ? 4095 : value));
            dac_channels[0].deadline_us = tick + DAC_SINE_TICK_US;
        } else if (mode == DAC_MODE_SQUARE) {
            const uint32_t frequency = dac_frequency_hz;
            const uint8_t duty = dac_duty_percent;
            const uint32_t period_us = frequency > 0 ? (1000000U + frequency / 2U) / frequency : 0;
            const uint32_t high_us = (period_us * duty + 50U) / 100U;
            const uint32_t low_us = period_us > high_us ? period_us - high_us : 0;
            dac_write_level(0, dac_channels[0].level);
            dac_channels[0].deadline_us += dac_channels[0].level ? high_us : low_us;
            dac_channels[0].level = !dac_channels[0].level;
        } else if (mode == DAC_MODE_SEQUENCE) {
            for (uint32_t index = 0; index < 2; ++index) {
                if (!dac_channels[index].active) continue;
                if (dac_channels[index].deadline_us > write_start + write_estimate + DAC_WRITE_MARGIN) {
                    continue;
                }
                if (dac_advance_sequence_channel(index)) {
                    dac_write_level(index, dac_channels[index].level);
                } else {
                    dac_channels[index].active = false;
                    dac_write_level(index, false);
                }
            }
        }

        const uint32_t write_duration = dac_now_us() - write_start;
        if (write_duration > 0 && write_duration < 2000U) {
            write_estimate = (3U * write_estimate + write_duration) / 4U + 2U;
            if (write_estimate > 1000U) write_estimate = 1000U;
        }
        if (write_duration >= 2000U && now > last_error_log_us + 5000000U) {
            last_error_log_us = now;
            EXTRA_WARN("DAC write stalled (%lu us)", (unsigned long)write_duration);
        }
    }
}

static esp_err_t dac_sequence_start_channel(const gpio_wave_segment_t *segments,
                                            size_t segment_count, bool loop,
                                            uint32_t channel_index)
{
    if (segments == NULL && segment_count > 0) return ESP_ERR_INVALID_ARG;
    if (segment_count > DAC_MAX_SEGMENTS) return ESP_ERR_INVALID_ARG;

    dac_channel_t *channel = &dac_channels[channel_index];
    memset(channel, 0, sizeof(*channel));
    if (segment_count > 0) {
        memcpy(channel->segments, segments, segment_count * sizeof(gpio_wave_segment_t));
    }
    channel->segment_count = segment_count;
    channel->loop = loop;
    channel->active = segment_count > 0;
    channel->index = 0;
    if (segment_count > 0) {
        channel->level = segments[0].level;
        dac_write_level(channel_index, channel->level);
        const uint32_t duration = segments[0].duration_us < DAC_MIN_SEGMENT_US
                                      ? DAC_MIN_SEGMENT_US : segments[0].duration_us;
        channel->deadline_us = dac_now_us() + duration;
    }
    return ESP_OK;
}

/* ———————————————————————————————— public interface ———————————————————————————————— */

esp_err_t gpio_extra_init(void)
{
    return ESP_OK;
}

esp_err_t gpio_extra_set_level(bool level)
{
    dac_set_both_levels(level);
    return ESP_OK;
}

esp_err_t gpio_wave_init(uint32_t frequency_hz, uint8_t duty_percent)
{
    if (frequency_hz < 10 || frequency_hz > 20000 || duty_percent < 1 || duty_percent > 99) {
        return ESP_ERR_INVALID_ARG;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_CYD_DAC_I2C_SDA,
        .scl_io_num = CONFIG_CYD_DAC_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &dac_bus), EXTRA_TAG, "I2C bus setup failed");

    const uint8_t addresses[2] = { CONFIG_CYD_DAC_DAC_A_ADDRESS, CONFIG_CYD_DAC_DAC_B_ADDRESS };
    uint32_t found = 0;
    for (uint32_t index = 0; index < 2; ++index) {
        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addresses[index],
            .scl_speed_hz = CONFIG_CYD_DAC_I2C_FREQ_HZ,
        };
        ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(dac_bus, &device_config, &dac_devices[index]),
                            EXTRA_TAG, "I2C device add failed");
        if (i2c_master_probe(dac_bus, addresses[index], 2000) == ESP_OK) {
            ++found;
            EXTRA_INFO("MCP4725 found at 0x%02X (channel %lu)", addresses[index], (unsigned long)(index + 1));
        } else {
            EXTRA_ERROR("MCP4725 not found at 0x%02X (channel %lu)", addresses[index], (unsigned long)(index + 1));
        }
    }
    if (found == 0) {
        EXTRA_ERROR("No MCP4725 DAC detected on I2C (SDA=%d, SCL=%d)",
                    CONFIG_CYD_DAC_I2C_SDA, CONFIG_CYD_DAC_I2C_SCL);
        return ESP_ERR_NOT_FOUND;
    }

    dac_frequency_hz = frequency_hz;
    dac_duty_percent = duty_percent;
    dac_sine_style = false;
    dac_mode = DAC_MODE_IDLE;
    memset(dac_channels, 0, sizeof(dac_channels));

    const esp_timer_create_args_t timer_args = {
        .callback = dac_edge_timer_callback,
        .arg = NULL,
        .name = "dac_edge",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &dac_edge_timer),
                        EXTRA_TAG, "Edge timer create failed");

    const BaseType_t created = xTaskCreatePinnedToCore(dac_renderer_fn, "dac_renderer",
                                                       CONFIG_CYD_DAC_RENDERER_TASK_STACK, NULL,
                                                       CONFIG_CYD_DAC_RENDERER_TASK_PRIORITY,
                                                       &dac_renderer_task, 0);
    if (created != pdPASS) {
        EXTRA_ERROR("Renderer task creation failed");
        esp_timer_delete(dac_edge_timer);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t gpio_wave_start(void)
{
    dac_channels[0].level = false;
    dac_channels[0].deadline_us = dac_now_us();
    dac_write_level(1, false);
    dac_mode = dac_sine_style ? DAC_MODE_SINE : DAC_MODE_SQUARE;
    xTaskNotifyGive(dac_renderer_task);
    return ESP_OK;
}

esp_err_t gpio_wave_stop(void)
{
    dac_mode = DAC_MODE_IDLE;
    dac_channels[0].active = false;
    dac_channels[1].active = false;
    dac_set_both_levels(false);
    xTaskNotifyGive(dac_renderer_task);
    return ESP_OK;
}

esp_err_t gpio_wave_set_frequency(uint32_t frequency_hz)
{
    if (frequency_hz < 10 || frequency_hz > 20000) return ESP_ERR_INVALID_ARG;
    dac_frequency_hz = frequency_hz;
    return ESP_OK;
}

esp_err_t gpio_wave_set_duty(uint8_t duty_percent)
{
    if (duty_percent < 1 || duty_percent > 99) return ESP_ERR_INVALID_ARG;
    dac_duty_percent = duty_percent;
    return ESP_OK;
}

esp_err_t gpio_wave_set_sine_style(bool sine)
{
    dac_sine_style = sine;
    return ESP_OK;
}

esp_err_t gpio_wave_get_effective(uint32_t *frequency_hz, uint8_t *duty_percent)
{
    if (frequency_hz != NULL) *frequency_hz = dac_frequency_hz;
    if (duty_percent != NULL) *duty_percent = dac_duty_percent;
    return ESP_OK;
}

esp_err_t gpio_sequence_start(const gpio_wave_segment_t *segments,
                              size_t segment_count, bool loop)
{
    ESP_RETURN_ON_ERROR(dac_sequence_start_channel(segments, segment_count, loop, 0),
                        EXTRA_TAG, "Sequence start failed");
    dac_channels[1].active = false;
    dac_mode = DAC_MODE_SEQUENCE;
    xTaskNotifyGive(dac_renderer_task);
    return ESP_OK;
}

esp_err_t gpio_sequence_stop(void)
{
    dac_mode = DAC_MODE_IDLE;
    dac_channels[0].active = false;
    dac_channels[1].active = false;
    dac_set_both_levels(false);
    xTaskNotifyGive(dac_renderer_task);
    return ESP_OK;
}

esp_err_t gpio_sequence_pair_start(const gpio_wave_segment_t *channel_1_segments,
                                   size_t channel_1_segment_count,
                                   const gpio_wave_segment_t *channel_2_segments,
                                   size_t channel_2_segment_count,
                                   bool loop)
{
    ESP_RETURN_ON_ERROR(dac_sequence_start_channel(channel_1_segments, channel_1_segment_count, loop, 0),
                        EXTRA_TAG, "Pair channel 1 start failed");
    ESP_RETURN_ON_ERROR(dac_sequence_start_channel(channel_2_segments, channel_2_segment_count, loop, 1),
                        EXTRA_TAG, "Pair channel 2 start failed");
    dac_mode = DAC_MODE_SEQUENCE;
    xTaskNotifyGive(dac_renderer_task);
    return ESP_OK;
}

esp_err_t gpio_sequence_pair_stop(void)
{
    return gpio_sequence_stop();
}

esp_err_t gpio_sync_test_start(void)
{
    const gpio_wave_segment_t channel_1[] = {
        { .level = true,  .duration_us = 300U },
        { .level = false, .duration_us = 3999700U },
    };
    const gpio_wave_segment_t channel_2[] = {
        { .level = false, .duration_us = 100U },
        { .level = true,  .duration_us = 300U },
        { .level = false, .duration_us = 3999600U },
    };
    ESP_RETURN_ON_ERROR(gpio_sequence_pair_start(channel_1, 2, channel_2, 3, true),
                        EXTRA_TAG, "Sync test start failed");
    return ESP_OK;
}

esp_err_t gpio_sync_test_stop(void)
{
    return gpio_sequence_stop();
}

esp_err_t gpio_burst_start(uint32_t frequency_hz, uint8_t duty_percent,
                           uint8_t pulse_count, uint16_t pause_ms)
{
    gpio_wave_segment_t segments[DAC_MAX_SEGMENTS];
    const uint32_t period_us = (1000000U + frequency_hz / 2U) / frequency_hz;
    const uint32_t high_us = (period_us * duty_percent + 50U) / 100U;
    size_t count = 0;
    for (uint8_t pulse = 0; pulse < pulse_count && count + 1U < DAC_MAX_SEGMENTS; ++pulse) {
        segments[count++] = (gpio_wave_segment_t){ .level = true, .duration_us = high_us };
        if (pulse + 1U < pulse_count) {
            segments[count++] = (gpio_wave_segment_t){ .level = false, .duration_us = period_us - high_us };
        }
    }
    segments[count++] = (gpio_wave_segment_t){ .level = false, .duration_us = (uint32_t)pause_ms * 1000U };
    return gpio_sequence_start(segments, count, true);
}

esp_err_t gpio_burst_stop(void)
{
    return gpio_sequence_stop();
}

esp_err_t gpio_burst_get_timing_us(uint32_t frequency_hz, uint8_t duty_percent,
                                   uint32_t *period_us, uint32_t *high_time_us)
{
    if (frequency_hz == 0 || duty_percent > 100) return ESP_ERR_INVALID_ARG;
    const uint32_t period = (1000000U + frequency_hz / 2U) / frequency_hz;
    if (period_us != NULL) *period_us = period;
    if (high_time_us != NULL) *high_time_us = (period * duty_percent + 50U) / 100U;
    return ESP_OK;
}