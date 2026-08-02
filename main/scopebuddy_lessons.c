#include "scopebuddy_lessons.h"
#include "esp_random.h"
#include <stdio.h>
#include <string.h>

static const scope_lesson_definition_t lessons[SCOPEBUDDY_LESSON_COUNT] = {
    { SCOPE_LESSON_PERIODIC, "PERIODISCH", "GRUNDLAGEN",
      "Frequenz, Periode und Tastgrad", 0x2684FF },
    { SCOPE_LESSON_PULSE_WIDTH, "PULSBREITEN", "GRUNDLAGEN",
      "High- und Low-Zeit sicher messen", 0x2684FF },
    { SCOPE_LESSON_BURST, "BURST", "PULSFOLGEN",
      "Pulszahl, Burstdauer und Pause", 0xD59A28 },
    { SCOPE_LESSON_MISSING_PULSE, "PULSLÜCKE", "EREIGNISSE",
      "Einen fehlenden Puls erkennen", 0xD96B44 },
    { SCOPE_LESSON_SERVO, "SERVOSIGNAL", "MAKERSIGNALE",
      "Pulsbreite in einen Winkel umrechnen", 0x3ABF91 },
    { SCOPE_LESSON_TACHOMETER, "TACHOSIGNAL", "MAKERSIGNALE",
      "Aus Pulsfrequenz die Drehzahl bestimmen", 0x3ABF91 },
    { SCOPE_LESSON_BUTTON_BOUNCE, "TASTERPRELLEN", "EREIGNISSE",
      "Kurze Flankenfolgen sichtbar machen", 0xD96B44 },
    { SCOPE_LESSON_UART, "UART 8N1", "KOMMUNIKATION",
      "Bitzeit, Baudrate und Datenbyte", 0x9A70E5 },
    { SCOPE_LESSON_ALTERNATING, "ZUSTANDSWECHSEL", "DYNAMISCHE SIGNALE",
      "Zwei Signalzustände unterscheiden", 0xC14F71 },
};

static uint32_t random_range(uint32_t minimum, uint32_t maximum)
{
    return minimum + esp_random() % (maximum - minimum + 1U);
}

static uint32_t choose(const uint32_t *values, size_t count)
{
    return values[esp_random() % count];
}

static void set_measurement(scope_lesson_instance_t *instance, uint8_t index,
                            const char *label, double tolerance, const char *format, double value)
{
    instance->measurements[index].label = label;
    instance->measurements[index].tolerance = tolerance;
    snprintf(instance->measurements[index].value,
             sizeof(instance->measurements[index].value), format, value);
}

static void set_measurement_u32(scope_lesson_instance_t *instance, uint8_t index,
                                const char *label, double tolerance,
                                const char *format, uint32_t value)
{
    instance->measurements[index].label = label;
    instance->measurements[index].tolerance = tolerance;
    snprintf(instance->measurements[index].value,
             sizeof(instance->measurements[index].value), format, (unsigned long)value);
}

static esp_err_t append_segment(scope_lesson_instance_t *instance, bool level, uint32_t duration_us)
{
    scope_sequence_spec_t *sequence = &instance->signal.data.sequence;
    if (duration_us == 0) return ESP_ERR_INVALID_ARG;
    if (sequence->segment_count > 0 &&
        sequence->segments[sequence->segment_count - 1U].level == level) {
        sequence->segments[sequence->segment_count - 1U].duration_us += duration_us;
        return ESP_OK;
    }
    if (sequence->segment_count >= SCOPEBUDDY_MAX_SEGMENTS) return ESP_ERR_INVALID_SIZE;
    sequence->segments[sequence->segment_count++] = (gpio_wave_segment_t) {
        .level = level,
        .duration_us = duration_us,
    };
    return ESP_OK;
}

static void format_pwm_measurements(scope_lesson_instance_t *instance)
{
    double frequency = instance->actual_frequency_a_hz;
    double period_us = 1000000.0 / frequency;
    double high_us = period_us * instance->actual_duty_a_percent / 100.0;
    double low_us = period_us - high_us;

    switch (instance->lesson->id) {
    case SCOPE_LESSON_PERIODIC:
        set_measurement(instance, 0, "Frequenz", frequency * 0.01, "%.2f Hz", frequency);
        set_measurement(instance, 1, "Periodendauer", period_us * 0.02, "%.2f µs", period_us);
        set_measurement_u32(instance, 2, "Tastgrad", 1.0, "%lu %%",
                            instance->actual_duty_a_percent);
        break;
    case SCOPE_LESSON_PULSE_WIDTH:
        set_measurement(instance, 0, "High-Pulsbreite", high_us * 0.03, "%.2f µs", high_us);
        set_measurement(instance, 1, "Low-Pulsbreite", low_us * 0.03, "%.2f µs", low_us);
        set_measurement_u32(instance, 2, "Tastgrad", 1.0, "%lu %%",
                            instance->actual_duty_a_percent);
        break;
    case SCOPE_LESSON_TACHOMETER: {
        uint32_t rpm = (uint32_t)((60U * instance->actual_frequency_a_hz +
                                   instance->auxiliary_a / 2U) / instance->auxiliary_a);
        set_measurement(instance, 0, "Pulsfrequenz", frequency * 0.02, "%.2f Hz", frequency);
        set_measurement(instance, 1, "Periodendauer", period_us * 0.03, "%.2f µs", period_us);
        set_measurement_u32(instance, 2, "Drehzahl", rpm * 0.03, "%lu U/min", rpm);
        break;
    }
    case SCOPE_LESSON_ALTERNATING:
        set_measurement_u32(instance, 0, "Frequenz A", frequency * 0.02, "%lu Hz",
                            instance->actual_frequency_a_hz);
        set_measurement_u32(instance, 1, "Frequenz B", instance->actual_frequency_b_hz * 0.02,
                            "%lu Hz", instance->actual_frequency_b_hz);
        set_measurement_u32(instance, 2, "Zustandsdauer A/B", instance->auxiliary_a * 0.03,
                            "%lu ms", instance->auxiliary_a);
        break;
    default:
        break;
    }
}

static esp_err_t generate_pwm(scope_lesson_instance_t *instance)
{
    static const uint32_t easy_frequencies[] = { 100, 200, 500, 1000 };
    static const uint32_t easy_duties[] = { 25, 50, 75 };
    uint32_t frequency;
    uint32_t duty;
    if (instance->difficulty == 1) {
        frequency = choose(easy_frequencies, 4);
        duty = choose(easy_duties, 3);
    } else if (instance->difficulty == 2) {
        frequency = random_range(500, 5000) / 50U * 50U;
        duty = random_range(2, 18) * 5U;
    } else {
        frequency = random_range(500, 10000);
        duty = random_range(10, 90);
    }
    instance->signal.kind = SCOPE_SIGNAL_PWM;
    instance->signal.data.pwm.frequency_hz = frequency;
    instance->signal.data.pwm.duty_percent = (uint8_t)duty;
    instance->actual_frequency_a_hz = frequency;
    instance->actual_duty_a_percent = (uint8_t)duty;
    snprintf(instance->context, sizeof(instance->context),
             "Miss das periodische Digitalsignal an GPIO48 gegen GND.");
    format_pwm_measurements(instance);
    return ESP_OK;
}

static esp_err_t generate_burst(scope_lesson_instance_t *instance)
{
    uint32_t frequency = instance->difficulty == 1 ? random_range(2, 10) * 100U :
                         instance->difficulty == 2 ? random_range(100, 1500) :
                                                     random_range(100, 2000);
    uint32_t duty = instance->difficulty == 1 ? 50U : random_range(20, 80);
    uint32_t pulse_count = random_range(3, instance->difficulty == 1 ? 7 : 15);
    uint32_t pause_ms = random_range(20, instance->difficulty == 3 ? 100 : 60);
    uint32_t period_us = (1000000U + frequency / 2U) / frequency;
    uint32_t high_us = (period_us * duty + 50U) / 100U;
    if (high_us == 0) high_us = 1;
    if (high_us >= period_us) high_us = period_us - 1U;

    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE;
    instance->signal.data.sequence.loop = true;
    for (uint32_t pulse = 0; pulse < pulse_count; ++pulse) {
        ESP_RETURN_ON_ERROR(append_segment(instance, true, high_us), "LESSONS", "Burst HIGH failed");
        if (pulse + 1U < pulse_count) {
            ESP_RETURN_ON_ERROR(append_segment(instance, false, period_us - high_us),
                                "LESSONS", "Burst LOW failed");
        }
    }
    ESP_RETURN_ON_ERROR(append_segment(instance, false, pause_ms * 1000U),
                        "LESSONS", "Burst pause failed");
    instance->period_us = period_us;
    instance->high_time_us = high_us;
    instance->auxiliary_a = pulse_count;
    instance->auxiliary_b = pause_ms;
    snprintf(instance->context, sizeof(instance->context),
             "Bestimme das Impulspaket und die Low-Pause bis zum nächsten Burst.");
    set_measurement_u32(instance, 0, "Pulse je Burst", 0, "%lu", pulse_count);
    set_measurement(instance, 1, "Burstdauer (Flanke-Flanke)", period_us * 0.003,
                    "%.3f ms", ((pulse_count - 1U) * period_us + high_us) / 1000.0);
    set_measurement_u32(instance, 2, "Low-Pause", pause_ms * 0.03, "%lu ms", pause_ms);
    return ESP_OK;
}

static esp_err_t generate_missing_pulse(scope_lesson_instance_t *instance)
{
    uint32_t frequency = instance->difficulty == 1 ? choose((uint32_t[]){200, 500, 1000}, 3) :
                         random_range(300, instance->difficulty == 2 ? 1500 : 2500);
    uint32_t period_us = (1000000U + frequency / 2U) / frequency;
    uint32_t high_us = period_us / 2U;
    uint32_t slots = random_range(6, instance->difficulty == 1 ? 8 : 12);
    uint32_t missing = random_range(1, slots - 2U);
    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE;
    instance->signal.data.sequence.loop = true;
    for (uint32_t slot = 0; slot < slots; ++slot) {
        if (slot == missing) {
            ESP_RETURN_ON_ERROR(append_segment(instance, false, period_us),
                                "LESSONS", "Pulse gap failed");
        } else {
            ESP_RETURN_ON_ERROR(append_segment(instance, true, high_us),
                                "LESSONS", "Pulse gap HIGH failed");
            ESP_RETURN_ON_ERROR(append_segment(instance, false, period_us - high_us),
                                "LESSONS", "Pulse gap LOW failed");
        }
    }
    snprintf(instance->context, sizeof(instance->context),
             "In jeder Folge fehlt genau ein Puls. Suche die vergrößerte Flankenlücke.");
    set_measurement_u32(instance, 0, "Grundperiode", period_us * 0.03, "%lu µs", period_us);
    set_measurement_u32(instance, 1, "Lücke steigende Flanken", period_us * 0.05,
                        "%lu µs", period_us * 2U);
    set_measurement_u32(instance, 2, "Sichtbare Pulse je Folge", 0, "%lu", slots - 1U);
    instance->period_us = period_us;
    instance->auxiliary_a = slots;
    instance->auxiliary_b = missing;
    return ESP_OK;
}

static esp_err_t generate_servo(scope_lesson_instance_t *instance)
{
    uint32_t step = instance->difficulty == 1 ? 500U : instance->difficulty == 2 ? 250U : 100U;
    uint32_t pulse_us = 1000U + random_range(0, 1000U / step) * step;
    uint32_t angle = (pulse_us - 1000U) * 180U / 1000U;
    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE;
    instance->signal.data.sequence.loop = true;
    ESP_RETURN_ON_ERROR(append_segment(instance, true, pulse_us), "LESSONS", "Servo HIGH failed");
    ESP_RETURN_ON_ERROR(append_segment(instance, false, 20000U - pulse_us),
                        "LESSONS", "Servo LOW failed");
    snprintf(instance->context, sizeof(instance->context),
             "Ein Modellbauservo bildet 1,0–2,0 ms linear auf 0–180° ab.");
    set_measurement_u32(instance, 0, "Wiederholperiode", 300, "%lu µs", 20000U);
    set_measurement_u32(instance, 1, "High-Pulsbreite", 30, "%lu µs", pulse_us);
    set_measurement_u32(instance, 2, "Sollwinkel", 3, "%lu°", angle);
    instance->period_us = 20000U;
    instance->high_time_us = pulse_us;
    return ESP_OK;
}

static esp_err_t generate_tachometer(scope_lesson_instance_t *instance)
{
    static const uint32_t ppr_values[] = { 1, 2, 4 };
    uint32_t ppr = ppr_values[instance->difficulty - 1U];
    uint32_t frequency = instance->difficulty == 1 ? random_range(5, 20) * 10U :
                         random_range(50, instance->difficulty == 2 ? 400 : 800);
    instance->signal.kind = SCOPE_SIGNAL_PWM;
    instance->signal.data.pwm = (scope_pwm_spec_t){ frequency, 50 };
    instance->actual_frequency_a_hz = frequency;
    instance->actual_duty_a_percent = 50;
    instance->auxiliary_a = ppr;
    snprintf(instance->context, sizeof(instance->context),
             "Der Tachosensor liefert %lu Impuls%s pro Umdrehung.",
             (unsigned long)ppr, ppr == 1 ? "" : "e");
    format_pwm_measurements(instance);
    return ESP_OK;
}

static esp_err_t generate_button_bounce(scope_lesson_instance_t *instance)
{
    uint32_t edges = instance->difficulty * 2U + 1U;
    uint32_t bounce_duration = 0;
    bool level = false;
    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE;
    instance->signal.data.sequence.loop = true;
    ESP_RETURN_ON_ERROR(append_segment(instance, false, 30000U), "LESSONS", "Bounce idle failed");
    for (uint32_t edge = 0; edge < edges; ++edge) {
        level = !level;
        uint32_t duration = edge + 1U < edges ? random_range(150, 1200) : 60000U;
        ESP_RETURN_ON_ERROR(append_segment(instance, level, duration),
                            "LESSONS", "Bounce edge failed");
        if (edge + 1U < edges) bounce_duration += duration;
    }
    snprintf(instance->context, sizeof(instance->context),
             "Miss den simulierten Schließvorgang vom ersten bis zum letzten Prellsprung.");
    set_measurement_u32(instance, 0, "Prellflanken", 0, "%lu", edges);
    set_measurement(instance, 1, "Prelldauer", bounce_duration * 0.05,
                    "%.3f ms", bounce_duration / 1000.0);
    set_measurement_u32(instance, 2, "Stabiler Endpegel", 0, "%lu (HIGH)", 1);
    instance->auxiliary_a = edges;
    instance->auxiliary_b = bounce_duration;
    return ESP_OK;
}

static esp_err_t generate_uart(scope_lesson_instance_t *instance)
{
    static const uint32_t baud_by_level[] = { 2400, 9600, 19200 };
    static const uint8_t bytes[] = { 0x35, 0x55, 0x6D, 0xA5, 0xC3 };
    uint32_t baud = baud_by_level[instance->difficulty - 1U];
    uint32_t bit_us = (1000000U + baud / 2U) / baud;
    uint8_t data = bytes[esp_random() % sizeof(bytes)];
    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE;
    instance->signal.data.sequence.loop = true;
    ESP_RETURN_ON_ERROR(append_segment(instance, false, bit_us), "LESSONS", "UART start failed");
    for (uint8_t bit = 0; bit < 8; ++bit) {
        ESP_RETURN_ON_ERROR(append_segment(instance, (data >> bit) & 1U, bit_us),
                            "LESSONS", "UART data failed");
    }
    ESP_RETURN_ON_ERROR(append_segment(instance, true, bit_us + 20000U),
                        "LESSONS", "UART stop failed");
    snprintf(instance->context, sizeof(instance->context),
             "UART 8N1, Idle HIGH, LSB zuerst. Dekodiere das wiederholte Byte.");
    set_measurement_u32(instance, 0, "Bitzeit", bit_us * 0.05, "%lu µs", bit_us);
    set_measurement_u32(instance, 1, "Baudrate (nominell)", baud * 0.03, "%lu Bd", baud);
    instance->measurements[2].label = "Datenbyte";
    instance->measurements[2].tolerance = 0;
    snprintf(instance->measurements[2].value, sizeof(instance->measurements[2].value),
             "0x%02X", data);
    instance->period_us = bit_us;
    instance->auxiliary_a = baud;
    instance->auxiliary_b = data;
    return ESP_OK;
}

static esp_err_t generate_alternating(scope_lesson_instance_t *instance)
{
    uint32_t frequency_a = random_range(2, instance->difficulty == 1 ? 10 : 60) * 100U;
    uint32_t frequency_b;
    do frequency_b = random_range(2, instance->difficulty == 1 ? 10 : 80) * 100U;
    while (frequency_b > frequency_a * 4U / 5U && frequency_b < frequency_a * 6U / 5U);
    uint32_t duration = instance->difficulty == 1 ? random_range(4, 10) * 100U :
                        random_range(150, 900);
    instance->signal.kind = SCOPE_SIGNAL_ALTERNATING;
    instance->signal.data.alternating.state_a = (scope_pwm_spec_t){ frequency_a, 50 };
    instance->signal.data.alternating.state_b = (scope_pwm_spec_t){ frequency_b,
        (uint8_t)(instance->difficulty == 3 ? random_range(25, 75) : 50) };
    instance->signal.data.alternating.state_duration_ms = (uint16_t)duration;
    instance->actual_frequency_a_hz = frequency_a;
    instance->actual_frequency_b_hz = frequency_b;
    instance->actual_duty_a_percent = 50;
    instance->actual_duty_b_percent = instance->signal.data.alternating.state_b.duty_percent;
    instance->auxiliary_a = duration;
    snprintf(instance->context, sizeof(instance->context),
             "Das Signal wechselt regelmäßig zwischen Zustand A und Zustand B.");
    format_pwm_measurements(instance);
    return ESP_OK;
}

static uint32_t calculate_signature(const scope_lesson_instance_t *instance)
{
    uint32_t value = (uint32_t)instance->lesson->id * 2654435761U;
    value ^= instance->difficulty << 24;
    value ^= instance->actual_frequency_a_hz * 2246822519U;
    value ^= instance->actual_frequency_b_hz * 668265263U;
    value ^= (uint32_t)instance->actual_duty_a_percent << 8;
    value ^= instance->period_us * 3266489917U;
    value ^= instance->high_time_us * 374761393U;
    value ^= instance->auxiliary_a | (instance->auxiliary_b << 16);
    return value;
}

const scope_lesson_definition_t *scopebuddy_lesson_at(size_t index)
{
    return index < SCOPEBUDDY_LESSON_COUNT ? &lessons[index] : NULL;
}

esp_err_t scopebuddy_generate_lesson(scope_lesson_id_t id, uint8_t difficulty,
                                     scope_lesson_instance_t *instance)
{
    if (instance == NULL || id >= SCOPEBUDDY_LESSON_COUNT || difficulty < 1 || difficulty > 3) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(instance, 0, sizeof(*instance));
    instance->lesson = &lessons[id];
    instance->difficulty = difficulty;
    instance->measurement_count = SCOPEBUDDY_MAX_MEASUREMENTS;
    esp_err_t err;
    switch (id) {
    case SCOPE_LESSON_PERIODIC:
    case SCOPE_LESSON_PULSE_WIDTH:
        err = generate_pwm(instance);
        break;
    case SCOPE_LESSON_BURST:
        err = generate_burst(instance);
        break;
    case SCOPE_LESSON_MISSING_PULSE:
        err = generate_missing_pulse(instance);
        break;
    case SCOPE_LESSON_SERVO:
        err = generate_servo(instance);
        break;
    case SCOPE_LESSON_TACHOMETER:
        err = generate_tachometer(instance);
        break;
    case SCOPE_LESSON_BUTTON_BOUNCE:
        err = generate_button_bounce(instance);
        break;
    case SCOPE_LESSON_UART:
        err = generate_uart(instance);
        break;
    case SCOPE_LESSON_ALTERNATING:
        err = generate_alternating(instance);
        break;
    default:
        err = ESP_ERR_INVALID_ARG;
    }
    if (err == ESP_OK) instance->signature = calculate_signature(instance);
    return err;
}

void scopebuddy_update_effective_pwm(scope_lesson_instance_t *instance,
                                     uint32_t frequency_a_hz, uint8_t duty_a_percent,
                                     uint32_t frequency_b_hz, uint8_t duty_b_percent)
{
    if (instance == NULL) return;
    instance->actual_frequency_a_hz = frequency_a_hz;
    instance->actual_duty_a_percent = duty_a_percent;
    if (frequency_b_hz > 0) instance->actual_frequency_b_hz = frequency_b_hz;
    if (duty_b_percent > 0) instance->actual_duty_b_percent = duty_b_percent;
    format_pwm_measurements(instance);
}
