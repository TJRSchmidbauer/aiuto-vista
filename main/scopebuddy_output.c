// Signal generation for the measurement tasks.
// Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
#include "scopebuddy_output.h"

#include "bsp_extra.h"
#include "esp_check.h"

static void convert_timeline(const scope_timeline_spec_t *source,
                             gpio_wave_segment_t destination[SCOPEBUDDY_MAX_SEGMENTS])
{
    for (size_t index = 0; index < source->segment_count; ++index) {
        destination[index].level = source->segments[index].level;
        destination[index].duration_us = source->segments[index].duration_us;
    }
}

esp_err_t scopebuddy_output_start(const scope_signal_spec_t *signal)
{
    if (signal == NULL) return ESP_ERR_INVALID_ARG;

    switch (signal->kind) {
    case SCOPE_SIGNAL_PWM:
        ESP_RETURN_ON_ERROR(gpio_wave_set_frequency(signal->data.pwm.frequency_hz),
                            "OUTPUT", "Setting frequency failed");
        ESP_RETURN_ON_ERROR(gpio_wave_set_duty(signal->data.pwm.duty_percent),
                            "OUTPUT", "Setting duty failed");
        return gpio_wave_start();
    case SCOPE_SIGNAL_SEQUENCE: {
        gpio_wave_segment_t segments[SCOPEBUDDY_MAX_SEGMENTS];
        convert_timeline(&signal->data.sequence.timeline, segments);
        return gpio_sequence_start(segments, signal->data.sequence.timeline.segment_count,
                                   signal->data.sequence.loop);
    }
    case SCOPE_SIGNAL_SEQUENCE_PAIR: {
        gpio_wave_segment_t channel_1[SCOPEBUDDY_MAX_SEGMENTS];
        gpio_wave_segment_t channel_2[SCOPEBUDDY_MAX_SEGMENTS];
        convert_timeline(&signal->data.pair.channels[0], channel_1);
        convert_timeline(&signal->data.pair.channels[1], channel_2);
        return gpio_sequence_pair_start(channel_1, signal->data.pair.channels[0].segment_count,
                                        channel_2, signal->data.pair.channels[1].segment_count,
                                        signal->data.pair.loop);
    }
    case SCOPE_SIGNAL_ALTERNATING:
        ESP_RETURN_ON_ERROR(gpio_wave_set_frequency(signal->data.alternating.state_a.frequency_hz),
                            "OUTPUT", "Setting state A frequency failed");
        ESP_RETURN_ON_ERROR(gpio_wave_set_duty(signal->data.alternating.state_a.duty_percent),
                            "OUTPUT", "Setting state A duty failed");
        return gpio_wave_start();
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t scopebuddy_output_stop(void)
{
    esp_err_t sequence_error = gpio_sequence_stop();
    esp_err_t wave_error = gpio_wave_stop();
    return sequence_error != ESP_OK ? sequence_error : wave_error;
}
