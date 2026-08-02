#ifndef SCOPEBUDDY_LESSONS_H
#define SCOPEBUDDY_LESSONS_H

#include "bsp_extra.h"
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#define SCOPEBUDDY_LESSON_COUNT 9U
#define SCOPEBUDDY_MAX_MEASUREMENTS 3U
#define SCOPEBUDDY_MAX_SEGMENTS 40U

typedef enum {
    SCOPE_LESSON_PERIODIC,
    SCOPE_LESSON_PULSE_WIDTH,
    SCOPE_LESSON_BURST,
    SCOPE_LESSON_MISSING_PULSE,
    SCOPE_LESSON_SERVO,
    SCOPE_LESSON_TACHOMETER,
    SCOPE_LESSON_BUTTON_BOUNCE,
    SCOPE_LESSON_UART,
    SCOPE_LESSON_ALTERNATING,
} scope_lesson_id_t;

typedef enum {
    SCOPE_SIGNAL_PWM,
    SCOPE_SIGNAL_SEQUENCE,
    SCOPE_SIGNAL_ALTERNATING,
} scope_signal_kind_t;

typedef struct {
    scope_lesson_id_t id;
    const char *title;
    const char *category;
    const char *summary;
    uint32_t accent;
} scope_lesson_definition_t;

typedef struct {
    const char *label;
    char value[32];
    double tolerance;
} scope_measurement_t;

typedef struct {
    uint32_t frequency_hz;
    uint8_t duty_percent;
} scope_pwm_spec_t;

typedef struct {
    gpio_wave_segment_t segments[SCOPEBUDDY_MAX_SEGMENTS];
    size_t segment_count;
    bool loop;
} scope_sequence_spec_t;

typedef struct {
    scope_pwm_spec_t state_a;
    scope_pwm_spec_t state_b;
    uint16_t state_duration_ms;
} scope_alternating_spec_t;

typedef struct {
    scope_signal_kind_t kind;
    union {
        scope_pwm_spec_t pwm;
        scope_sequence_spec_t sequence;
        scope_alternating_spec_t alternating;
    } data;
} scope_signal_spec_t;

typedef struct {
    const scope_lesson_definition_t *lesson;
    uint8_t difficulty;
    char context[112];
    scope_signal_spec_t signal;
    scope_measurement_t measurements[SCOPEBUDDY_MAX_MEASUREMENTS];
    uint8_t measurement_count;
    uint32_t actual_frequency_a_hz;
    uint32_t actual_frequency_b_hz;
    uint8_t actual_duty_a_percent;
    uint8_t actual_duty_b_percent;
    uint32_t period_us;
    uint32_t high_time_us;
    uint32_t auxiliary_a;
    uint32_t auxiliary_b;
    uint32_t signature;
} scope_lesson_instance_t;

const scope_lesson_definition_t *scopebuddy_lesson_at(size_t index);
esp_err_t scopebuddy_generate_lesson(scope_lesson_id_t id, uint8_t difficulty,
                                     scope_lesson_instance_t *instance);
void scopebuddy_update_effective_pwm(scope_lesson_instance_t *instance,
                                     uint32_t frequency_a_hz, uint8_t duty_a_percent,
                                     uint32_t frequency_b_hz, uint8_t duty_b_percent);

#endif
