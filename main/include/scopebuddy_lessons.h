#ifndef SCOPEBUDDY_LESSONS_H
#define SCOPEBUDDY_LESSONS_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCOPEBUDDY_LESSON_COUNT 9U
#define SCOPEBUDDY_MAX_CHANNELS 2U
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
    SCOPE_SIGNAL_SEQUENCE_PAIR,
    SCOPE_SIGNAL_ALTERNATING,
} scope_signal_kind_t;

typedef enum {
    SCOPE_MEASUREMENT_CHANNEL_1,
    SCOPE_MEASUREMENT_CHANNEL_2,
    SCOPE_MEASUREMENT_CHANNEL_PAIR,
    SCOPE_MEASUREMENT_DERIVED,
} scope_measurement_source_t;

typedef enum {
    SCOPE_MEASUREMENT_NUMERIC,
    SCOPE_MEASUREMENT_TEXT,
} scope_measurement_value_kind_t;

typedef struct {
    scope_lesson_id_t id;
    const char *title;
    const char *category;
    const char *summary;
    const char *learning_objective;
    const char *connection_hint;
    const char *trigger_hint;
    uint32_t accent;
    uint8_t required_channels;
} scope_lesson_definition_t;

typedef struct {
    const char *label;
    const char *unit;
    const char *calculation;
    scope_measurement_source_t source;
    scope_measurement_value_kind_t value_kind;
    double expected_value;
    double tolerance;
    uint8_t decimals;
    char value[32];
} scope_measurement_t;

typedef struct {
    uint32_t frequency_hz;
    uint8_t duty_percent;
} scope_pwm_spec_t;

typedef struct {
    bool level;
    uint32_t duration_us;
} scope_signal_segment_t;

typedef struct {
    scope_signal_segment_t segments[SCOPEBUDDY_MAX_SEGMENTS];
    size_t segment_count;
} scope_timeline_spec_t;

typedef struct {
    scope_timeline_spec_t timeline;
    bool loop;
} scope_sequence_spec_t;

typedef struct {
    scope_timeline_spec_t channels[SCOPEBUDDY_MAX_CHANNELS];
    bool loop;
} scope_sequence_pair_spec_t;

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
        scope_sequence_pair_spec_t pair;
        scope_alternating_spec_t alternating;
    } data;
} scope_signal_spec_t;

typedef struct {
    uint32_t frequency_hz;
    uint32_t period_us;
    uint32_t high_time_us;
    uint8_t duty_percent;
} scope_realized_channel_t;

typedef union {
    struct { uint32_t pulse_count; uint32_t pause_ms; } burst;
    struct { uint32_t slot_count; uint32_t missing_index; } missing_pulse;
    struct { uint32_t target_angle_deg; } servo;
    struct { uint32_t pulses_per_revolution; } tachometer;
    struct { uint32_t edge_count; uint32_t duration_us; } button_bounce;
    struct { uint32_t baud; uint8_t data_byte; } uart;
    struct { uint32_t state_duration_ms; } alternating;
} scope_lesson_parameters_t;

typedef struct {
    const scope_lesson_definition_t *lesson;
    uint8_t difficulty;
    char context[112];
    scope_signal_spec_t signal;
    scope_measurement_t measurements[SCOPEBUDDY_MAX_MEASUREMENTS];
    uint8_t measurement_count;
    scope_realized_channel_t realized[SCOPEBUDDY_MAX_CHANNELS];
    scope_lesson_parameters_t parameters;
    uint32_t signature;
} scope_lesson_instance_t;

size_t scopebuddy_lesson_count(void);
const scope_lesson_definition_t *scopebuddy_lesson_at(size_t index);
esp_err_t scopebuddy_generate_lesson(scope_lesson_id_t id, uint8_t difficulty,
                                     scope_lesson_instance_t *instance);
void scopebuddy_update_effective_pwm(scope_lesson_instance_t *instance,
                                     uint32_t frequency_a_hz, uint8_t duty_a_percent,
                                     uint32_t frequency_b_hz, uint8_t duty_b_percent);

#endif
