// Lesson and measurement-task definitions.
// Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
#include "scopebuddy_lessons.h"

#include "esp_check.h"
#include "esp_random.h"
#include <stdio.h>
#include <string.h>

#define CH1_CONNECTION "CH1 an GPIO26, Masse an GND."

static const scope_lesson_definition_t lessons[SCOPEBUDDY_LESSON_COUNT] = {
    { .id = SCOPE_LESSON_PERIODIC, .title = "PERIODISCHES SIGNAL", .category = "GRUNDLAGEN",
      .summary = "Frequenz, Periodendauer und Tastgrad",
      .learning_objective = "Frequenz, Periode und Tastgrad sicher bestimmen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Flankentrigger auf CH1 verwenden.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_PULSE_WIDTH, .title = "PULSBREITEN", .category = "GRUNDLAGEN",
      .summary = "High-Zeit, Low-Zeit und Tastgrad",
      .learning_objective = "High- und Low-Zeit eines Rechtecksignals messen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Auf eine steigende Flanke triggern.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_BURST, .title = "BURST", .category = "PULSFOLGEN",
      .summary = "Pulszahl, Burstdauer und Low-Pause",
      .learning_objective = "Ein Impulspaket zeitlich vollständig beschreiben.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Zeitbasis so wählen, dass zwei Bursts sichtbar sind.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_MISSING_PULSE, .title = "PULSLÜCKE", .category = "EREIGNISSE",
      .summary = "Grundperiode, Flankenlücke und Pulszahl",
      .learning_objective = "Eine vergrößerte Flankenlücke in einer Pulsfolge erkennen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Auf steigende Flanke triggern und mehrere Perioden zeigen.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_SERVO, .title = "SERVOSIGNAL", .category = "STEUERSIGNALE",
      .summary = "Wiederholperiode, Pulsbreite und Winkel",
      .learning_objective = "Die Pulsbreite eines Servosignals in einen Winkel umrechnen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Auf steigende Flanke triggern; Pulsbreitenmessung aktivieren.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_TACHOMETER, .title = "TACHOSIGNAL", .category = "SENSORSIGNALE",
      .summary = "Pulsfrequenz, Periodendauer und Drehzahl",
      .learning_objective = "Aus Frequenz und Impulsen je Umdrehung die Drehzahl berechnen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Stabil auf eine steigende Flanke triggern.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_BUTTON_BOUNCE, .title = "TASTERPRELLEN", .category = "EREIGNISSE",
      .summary = "Prellflanken, Prelldauer und Endpegel",
      .learning_objective = "Prellflanken zählen und die gesamte Prelldauer messen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Single-Shot auf die erste steigende Flanke einstellen.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_UART, .title = "UART 8N1", .category = "KOMMUNIKATION",
      .summary = "Bitzeit, Baudrate und Datenbyte",
      .learning_objective = "Ein UART-8N1-Byte aus dem Zeitverlauf dekodieren.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Auf die fallende Startbit-Flanke triggern.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_ALTERNATING, .title = "ZUSTANDSWECHSEL", .category = "DYNAMISCHE SIGNALE",
      .summary = "Frequenzen beider Zustände und Zustandsdauer",
      .learning_objective = "Zwei zeitlich wechselnde Signalzustände getrennt vermessen.",
      .connection_hint = CH1_CONNECTION, .trigger_hint = "Lange Zeitbasis wählen, danach beide Zustände vergrößern.",
      .required_channels = 1 },
    { .id = SCOPE_LESSON_TRIGGER_RESPONSE, .title = "TRIGGER-ANTWORT", .category = "EREIGNISSE",
      .summary = "Triggerbreite, Antwortbreite und Reaktionszeit",
      .learning_objective = "Pulsbreiten und die Verzögerung zwischen zwei Kanälen bestimmen.",
      .connection_hint = "CH1 an GPIO26, CH2 an GPIO27, gemeinsame Masse an GND.",
      .trigger_hint = "Auf die steigende Flanke von CH1 triggern.",
      .required_channels = 2 },
    { .id = SCOPE_LESSON_PHASE_SHIFT, .title = "PHASENVERSCHIEBUNG", .category = "ZEITBEZIEHUNGEN",
      .summary = "Periodendauer, Zeitversatz und Phasenwinkel",
      .learning_objective = "Aus Periodendauer und Zeitversatz die Phasenlage berechnen.",
      .connection_hint = "CH1 an GPIO26, CH2 an GPIO27, gemeinsame Masse an GND.",
      .trigger_hint = "Auf CH1 triggern und die steigenden Flanken beider Kanäle vergleichen.",
      .required_channels = 2 },
    { .id = SCOPE_LESSON_FREQUENCY_DIVIDER, .title = "FREQUENZTEILER", .category = "TAKTSIGNALE",
      .summary = "Ein- und Ausgangsfrequenz sowie Teilverhältnis",
      .learning_objective = "Zwei Frequenzen messen und daraus das ganzzahlige Teilverhältnis bestimmen.",
      .connection_hint = "CH1 an GPIO26, CH2 an GPIO27, gemeinsame Masse an GND.",
      .trigger_hint = "Auf CH2 triggern und mehrere CH1-Perioden anzeigen.",
      .required_channels = 2 },
    { .id = SCOPE_LESSON_ULTRASONIC, .title = "ULTRASCHALL-ECHO", .category = "SENSORSIGNALE",
      .summary = "Echoverzögerung, Echobreite und Entfernung",
      .learning_objective = "Aus der Breite eines simulierten Echosignals eine Entfernung bestimmen.",
      .connection_hint = "CH1 an GPIO26 (Trigger), CH2 an GPIO27 (Echo), Masse an GND.",
      .trigger_hint = "Auf die steigende Triggerflanke von CH1 triggern.",
      .required_channels = 2 },
    { .id = SCOPE_LESSON_GATED_PWM, .title = "FREIGEGEBENE PWM", .category = "STEUERSIGNALE",
      .summary = "Startverzögerung, Pulszahl und Freigabedauer",
      .learning_objective = "Startverzögerung, Pulszahl und Dauer eines freigegebenen PWM-Pakets messen.",
      .connection_hint = "CH1 an GPIO26 (Freigabe), CH2 an GPIO27 (PWM), Masse an GND.",
      .trigger_hint = "Auf die steigende Freigabeflanke von CH1 triggern.",
      .required_channels = 2 },
    { .id = SCOPE_LESSON_QUADRATURE, .title = "QUADRATURGEBER", .category = "ENCODERSIGNALE",
      .summary = "Periodendauer, Flankenversatz und führende Spur",
      .learning_objective = "Aus der führenden Spur die simulierte Drehrichtung bestimmen.",
      .connection_hint = "CH1 an GPIO26 (Spur A), CH2 an GPIO27 (Spur B), Masse an GND.",
      .trigger_hint = "Auf eine steigende Flanke von Spur A triggern.",
      .required_channels = 2 },
};

static uint32_t random_range(uint32_t minimum, uint32_t maximum)
{
    return minimum + esp_random() % (maximum - minimum + 1U);
}

static uint32_t choose(const uint32_t *values, size_t count)
{
    return values[esp_random() % count];
}

static void set_numeric_measurement(scope_lesson_instance_t *instance, uint8_t index,
                                    const char *label, scope_measurement_source_t source,
                                    double expected, double tolerance, const char *unit,
                                    uint8_t decimals, const char *calculation)
{
    scope_measurement_t *measurement = &instance->measurements[index];
    measurement->label = label;
    measurement->unit = unit;
    measurement->calculation = calculation;
    measurement->source = source;
    measurement->value_kind = SCOPE_MEASUREMENT_NUMERIC;
    measurement->expected_value = expected;
    measurement->tolerance = tolerance;
    measurement->decimals = decimals;
    snprintf(measurement->value, sizeof(measurement->value), "%.*f%s%s", decimals, expected,
             unit != NULL && unit[0] != '\0' ? " " : "", unit != NULL ? unit : "");
}

static void set_text_measurement(scope_lesson_instance_t *instance, uint8_t index,
                                 const char *label, scope_measurement_source_t source,
                                 const char *value, const char *calculation)
{
    scope_measurement_t *measurement = &instance->measurements[index];
    measurement->label = label;
    measurement->unit = "";
    measurement->calculation = calculation;
    measurement->source = source;
    measurement->value_kind = SCOPE_MEASUREMENT_TEXT;
    measurement->tolerance = 0;
    snprintf(measurement->value, sizeof(measurement->value), "%s", value);
}

static esp_err_t append_timeline_segment(scope_timeline_spec_t *timeline, bool level,
                                         uint32_t duration_us)
{
    if (timeline == NULL || duration_us == 0) return ESP_ERR_INVALID_ARG;
    if (timeline->segment_count > 0 &&
        timeline->segments[timeline->segment_count - 1U].level == level) {
        timeline->segments[timeline->segment_count - 1U].duration_us += duration_us;
        return ESP_OK;
    }
    if (timeline->segment_count >= SCOPEBUDDY_MAX_SEGMENTS) return ESP_ERR_INVALID_SIZE;
    timeline->segments[timeline->segment_count++] = (scope_signal_segment_t) {
        .level = level, .duration_us = duration_us,
    };
    return ESP_OK;
}

static esp_err_t append_segment(scope_lesson_instance_t *instance, bool level, uint32_t duration_us)
{
    return append_timeline_segment(&instance->signal.data.sequence.timeline, level, duration_us);
}

static esp_err_t append_pair_segment(scope_lesson_instance_t *instance, size_t channel,
                                     bool level, uint32_t duration_us)
{
    if (channel >= SCOPEBUDDY_MAX_CHANNELS) return ESP_ERR_INVALID_ARG;
    return append_timeline_segment(&instance->signal.data.pair.channels[channel], level, duration_us);
}

static void begin_pair(scope_lesson_instance_t *instance)
{
    instance->signal.kind = SCOPE_SIGNAL_SEQUENCE_PAIR;
    instance->signal.data.pair.loop = true;
}

static void set_realized_pwm(scope_realized_channel_t *channel, uint32_t frequency, uint8_t duty)
{
    channel->frequency_hz = frequency;
    channel->duty_percent = duty;
    channel->period_us = frequency > 0 ? (1000000U + frequency / 2U) / frequency : 0;
    channel->high_time_us = (channel->period_us * duty + 50U) / 100U;
}

static void format_pwm_measurements(scope_lesson_instance_t *instance)
{
    const scope_realized_channel_t *channel_a = &instance->realized[0];
    double frequency = channel_a->frequency_hz;
    double period_us = frequency > 0 ? 1000000.0 / frequency : 0;
    double high_us = period_us * channel_a->duty_percent / 100.0;
    double low_us = period_us - high_us;

    switch (instance->lesson->id) {
    case SCOPE_LESSON_PERIODIC:
        set_numeric_measurement(instance, 0, "Frequenz", SCOPE_MEASUREMENT_CHANNEL_1,
                                frequency, frequency * 0.01, "Hz", 2, "f = 1 / T");
        set_numeric_measurement(instance, 1, "Periodendauer", SCOPE_MEASUREMENT_CHANNEL_1,
                                period_us, period_us * 0.02, "µs", 2, "T = 1 / f");
        set_numeric_measurement(instance, 2, "Tastgrad", SCOPE_MEASUREMENT_CHANNEL_1,
                                channel_a->duty_percent, 1.0, "%", 0, "D = tHigh / T");
        break;
    case SCOPE_LESSON_PULSE_WIDTH:
        set_numeric_measurement(instance, 0, "High-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_1,
                                high_us, high_us * 0.03, "µs", 2, "tHigh");
        set_numeric_measurement(instance, 1, "Low-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_1,
                                low_us, low_us * 0.03, "µs", 2, "tLow = T - tHigh");
        set_numeric_measurement(instance, 2, "Tastgrad", SCOPE_MEASUREMENT_CHANNEL_1,
                                channel_a->duty_percent, 1.0, "%", 0, "D = tHigh / T");
        break;
    case SCOPE_LESSON_TACHOMETER: {
        uint32_t ppr = instance->parameters.tachometer.pulses_per_revolution;
        double rpm = ppr > 0 ? 60.0 * frequency / ppr : 0;
        set_numeric_measurement(instance, 0, "Pulsfrequenz", SCOPE_MEASUREMENT_CHANNEL_1,
                                frequency, frequency * 0.02, "Hz", 2, "f = 1 / T");
        set_numeric_measurement(instance, 1, "Periodendauer", SCOPE_MEASUREMENT_CHANNEL_1,
                                period_us, period_us * 0.03, "µs", 2, "T = 1 / f");
        set_numeric_measurement(instance, 2, "Drehzahl", SCOPE_MEASUREMENT_DERIVED,
                                rpm, rpm * 0.03, "U/min", 0, "n = 60 f / PPR");
        break;
    }
    case SCOPE_LESSON_ALTERNATING:
        set_numeric_measurement(instance, 0, "Frequenz A", SCOPE_MEASUREMENT_CHANNEL_1,
                                channel_a->frequency_hz, channel_a->frequency_hz * 0.02,
                                "Hz", 0, "fA = 1 / TA");
        set_numeric_measurement(instance, 1, "Frequenz B", SCOPE_MEASUREMENT_CHANNEL_1,
                                instance->realized[1].frequency_hz,
                                instance->realized[1].frequency_hz * 0.02,
                                "Hz", 0, "fB = 1 / TB");
        set_numeric_measurement(instance, 2, "Zustandsdauer A/B", SCOPE_MEASUREMENT_DERIVED,
                                instance->parameters.alternating.state_duration_ms,
                                instance->parameters.alternating.state_duration_ms * 0.03,
                                "ms", 0, "Zeit zwischen Zustandswechseln");
        break;
    default:
        break;
    }
}

static esp_err_t generate_pwm(scope_lesson_instance_t *instance)
{
    static const uint32_t easy_frequencies[] = { 100, 200, 500, 1000 };
    static const uint32_t easy_duties[] = { 25, 50, 75 };
    uint32_t frequency = instance->difficulty == 1 ? choose(easy_frequencies, 4) :
                         instance->difficulty == 2 ? random_range(500, 5000) / 50U * 50U :
                                                     random_range(500, 10000);
    uint32_t duty = instance->difficulty == 1 ? choose(easy_duties, 3) :
                    instance->difficulty == 2 ? random_range(2, 18) * 5U :
                                                random_range(10, 90);
    instance->signal.kind = SCOPE_SIGNAL_PWM;
    instance->signal.data.pwm = (scope_pwm_spec_t){ frequency, (uint8_t)duty };
    set_realized_pwm(&instance->realized[0], frequency, (uint8_t)duty);
    snprintf(instance->context, sizeof(instance->context),
             "Miss das periodische Digitalsignal an GPIO26 gegen GND.");
    format_pwm_measurements(instance);
    return ESP_OK;
}

static esp_err_t generate_burst(scope_lesson_instance_t *instance)
{
    uint32_t frequency = instance->difficulty == 1 ? random_range(2, 10) * 100U :
                         instance->difficulty == 2 ? random_range(100, 1500) : random_range(100, 2000);
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
    ESP_RETURN_ON_ERROR(append_segment(instance, false, pause_ms * 1000U), "LESSONS", "Burst pause failed");
    instance->realized[0] = (scope_realized_channel_t){ frequency, period_us, high_us, (uint8_t)duty };
    instance->parameters.burst.pulse_count = pulse_count;
    instance->parameters.burst.pause_ms = pause_ms;
    snprintf(instance->context, sizeof(instance->context),
             "Bestimme das Impulspaket und die Low-Pause bis zum nächsten Burst.");
    set_numeric_measurement(instance, 0, "Pulse je Burst", SCOPE_MEASUREMENT_CHANNEL_1,
                            pulse_count, 0, "", 0, "Pulse im Paket zählen");
    set_numeric_measurement(instance, 1, "Burstdauer (Flanke-Flanke)", SCOPE_MEASUREMENT_CHANNEL_1,
                            ((pulse_count - 1U) * period_us + high_us) / 1000.0,
                            (((pulse_count - 1U) * period_us + high_us) / 1000.0) * 0.03,
                            "ms", 3, "Vom ersten Anstieg bis zum letzten Abfall");
    set_numeric_measurement(instance, 2, "Low-Pause", SCOPE_MEASUREMENT_CHANNEL_1,
                            pause_ms, pause_ms * 0.03, "ms", 0, "Pause bis zum nächsten Burst");
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
            ESP_RETURN_ON_ERROR(append_segment(instance, false, period_us), "LESSONS", "Pulse gap failed");
        } else {
            ESP_RETURN_ON_ERROR(append_segment(instance, true, high_us), "LESSONS", "Pulse gap HIGH failed");
            ESP_RETURN_ON_ERROR(append_segment(instance, false, period_us - high_us), "LESSONS", "Pulse gap LOW failed");
        }
    }
    instance->realized[0] = (scope_realized_channel_t){ frequency, period_us, high_us, 50 };
    instance->parameters.missing_pulse.slot_count = slots;
    instance->parameters.missing_pulse.missing_index = missing;
    snprintf(instance->context, sizeof(instance->context),
             "In jeder Folge fehlt genau ein Puls. Suche die vergrößerte Flankenlücke.");
    set_numeric_measurement(instance, 0, "Grundperiode", SCOPE_MEASUREMENT_CHANNEL_1,
                            period_us, period_us * 0.03, "µs", 0, "Abstand normaler steigender Flanken");
    set_numeric_measurement(instance, 1, "Lücke steigende Flanken", SCOPE_MEASUREMENT_CHANNEL_1,
                            period_us * 2U, period_us * 0.05, "µs", 0, "Abstand über dem fehlenden Puls");
    set_numeric_measurement(instance, 2, "Sichtbare Pulse je Folge", SCOPE_MEASUREMENT_CHANNEL_1,
                            slots - 1U, 0, "", 0, "Slots minus Pulslücke");
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
    ESP_RETURN_ON_ERROR(append_segment(instance, false, 20000U - pulse_us), "LESSONS", "Servo LOW failed");
    instance->realized[0] = (scope_realized_channel_t){ 50, 20000, pulse_us, (uint8_t)((pulse_us + 100U) / 200U) };
    instance->parameters.servo.target_angle_deg = angle;
    snprintf(instance->context, sizeof(instance->context),
             "Ein Modellbauservo bildet 1,0–2,0 ms linear auf 0–180° ab.");
    set_numeric_measurement(instance, 0, "Wiederholperiode", SCOPE_MEASUREMENT_CHANNEL_1,
                            20000, 300, "µs", 0, "Periode zwischen zwei Pulsen");
    set_numeric_measurement(instance, 1, "High-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_1,
                            pulse_us, 30, "µs", 0, "Breite des HIGH-Pulses");
    set_numeric_measurement(instance, 2, "Sollwinkel", SCOPE_MEASUREMENT_DERIVED,
                            angle, 3, "°", 0, "α = (tHigh - 1 ms) · 180° / 1 ms");
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
    set_realized_pwm(&instance->realized[0], frequency, 50);
    instance->parameters.tachometer.pulses_per_revolution = ppr;
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
        ESP_RETURN_ON_ERROR(append_segment(instance, level, duration), "LESSONS", "Bounce edge failed");
        if (edge + 1U < edges) bounce_duration += duration;
    }
    instance->parameters.button_bounce.edge_count = edges;
    instance->parameters.button_bounce.duration_us = bounce_duration;
    snprintf(instance->context, sizeof(instance->context),
             "Miss den simulierten Schließvorgang vom ersten bis zum letzten Prellsprung.");
    set_numeric_measurement(instance, 0, "Prellflanken", SCOPE_MEASUREMENT_CHANNEL_1,
                            edges, 0, "", 0, "Flanken bis zum stabilen Endpegel zählen");
    set_numeric_measurement(instance, 1, "Prelldauer", SCOPE_MEASUREMENT_CHANNEL_1,
                            bounce_duration / 1000.0, bounce_duration * 0.00005,
                            "ms", 3, "Zeit vom ersten bis zum letzten Prellsprung");
    set_text_measurement(instance, 2, "Stabiler Endpegel", SCOPE_MEASUREMENT_CHANNEL_1,
                         "HIGH", "Pegel nach dem letzten Prellsprung");
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
        ESP_RETURN_ON_ERROR(append_segment(instance, (data >> bit) & 1U, bit_us), "LESSONS", "UART data failed");
    }
    ESP_RETURN_ON_ERROR(append_segment(instance, true, bit_us + 20000U), "LESSONS", "UART stop failed");
    instance->realized[0] = (scope_realized_channel_t){ baud, bit_us, 0, 0 };
    instance->parameters.uart.baud = baud;
    instance->parameters.uart.data_byte = data;
    snprintf(instance->context, sizeof(instance->context),
             "UART 8N1, Idle HIGH, LSB zuerst. Dekodiere das wiederholte Byte.");
    set_numeric_measurement(instance, 0, "Bitzeit", SCOPE_MEASUREMENT_CHANNEL_1,
                            bit_us, bit_us * 0.05, "µs", 0, "tBit = 1 / Baudrate");
    set_numeric_measurement(instance, 1, "Baudrate (nominell)", SCOPE_MEASUREMENT_DERIVED,
                            baud, baud * 0.03, "Bd", 0, "Baud = 1 / tBit");
    char byte_text[8];
    snprintf(byte_text, sizeof(byte_text), "0x%02X", data);
    set_text_measurement(instance, 2, "Datenbyte", SCOPE_MEASUREMENT_DERIVED,
                         byte_text, "Startbit, acht Datenbits LSB zuerst, Stopbit");
    return ESP_OK;
}

static esp_err_t generate_alternating(scope_lesson_instance_t *instance)
{
    uint32_t frequency_a = random_range(2, instance->difficulty == 1 ? 10 : 60) * 100U;
    uint32_t frequency_b;
    do frequency_b = random_range(2, instance->difficulty == 1 ? 10 : 80) * 100U;
    while (frequency_b > frequency_a * 4U / 5U && frequency_b < frequency_a * 6U / 5U);
    uint32_t duration = instance->difficulty == 1 ? random_range(4, 10) * 100U : random_range(150, 900);
    uint8_t duty_b = (uint8_t)(instance->difficulty == 3 ? random_range(25, 75) : 50);
    instance->signal.kind = SCOPE_SIGNAL_ALTERNATING;
    instance->signal.data.alternating.state_a = (scope_pwm_spec_t){ frequency_a, 50 };
    instance->signal.data.alternating.state_b = (scope_pwm_spec_t){ frequency_b, duty_b };
    instance->signal.data.alternating.state_duration_ms = (uint16_t)duration;
    set_realized_pwm(&instance->realized[0], frequency_a, 50);
    set_realized_pwm(&instance->realized[1], frequency_b, duty_b);
    instance->parameters.alternating.state_duration_ms = duration;
    snprintf(instance->context, sizeof(instance->context),
             "Das Signal wechselt regelmäßig zwischen Zustand A und Zustand B.");
    format_pwm_measurements(instance);
    return ESP_OK;
}

static esp_err_t generate_trigger_response(scope_lesson_instance_t *instance)
{
    static const uint32_t easy_delays[] = { 1000, 2000, 5000 };
    uint32_t trigger_width = instance->difficulty == 1 ? 500U :
                             instance->difficulty == 2 ? random_range(2, 10) * 100U :
                                                         random_range(100, 1500);
    uint32_t response_width = instance->difficulty == 1 ? 1000U :
                              instance->difficulty == 2 ? random_range(5, 20) * 100U :
                                                          random_range(200, 2500);
    uint32_t delay = instance->difficulty == 1 ? choose(easy_delays, 3) :
                     instance->difficulty == 2 ? random_range(5, 50) * 100U :
                                                 random_range(300, 10000);
    uint32_t common_duration = delay + response_width + 10000U;

    begin_pair(instance);
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, trigger_width),
                        "LESSONS", "Trigger pulse failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, common_duration - trigger_width),
                        "LESSONS", "Trigger idle failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, delay),
                        "LESSONS", "Response delay failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, response_width),
                        "LESSONS", "Response pulse failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, common_duration - delay - response_width),
                        "LESSONS", "Response idle failed");

    instance->realized[0] = (scope_realized_channel_t){ 0, common_duration, trigger_width, 0 };
    instance->realized[1] = (scope_realized_channel_t){ 0, common_duration, response_width, 0 };
    instance->parameters.trigger_response.delay_us = delay;
    instance->parameters.trigger_response.response_width_us = response_width;
    snprintf(instance->context, sizeof(instance->context),
             "CH1 löst ein Ereignis aus; CH2 antwortet nach einer definierten Reaktionszeit.");
    set_numeric_measurement(instance, 0, "Trigger-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_1,
                            trigger_width, trigger_width * 0.03, "µs", 0, "HIGH-Zeit auf CH1");
    set_numeric_measurement(instance, 1, "Antwort-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_2,
                            response_width, response_width * 0.03, "µs", 0, "HIGH-Zeit auf CH2");
    set_numeric_measurement(instance, 2, "Reaktionszeit", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                            delay, delay * 0.03, "µs", 0, "Steigende Flanke CH1 bis steigende Flanke CH2");
    return ESP_OK;
}

static esp_err_t generate_phase_shift(scope_lesson_instance_t *instance)
{
    static const uint32_t periods[] = { 1000, 2000, 4000 };
    static const uint32_t easy_phases[] = { 90, 180, 270 };
    static const uint32_t medium_phases[] = { 45, 90, 135, 180, 225, 270 };
    uint32_t period = choose(periods, 3);
    uint32_t requested_phase = instance->difficulty == 1 ? choose(easy_phases, 3) :
                               instance->difficulty == 2 ? choose(medium_phases, 6) :
                                                           random_range(2, 18) * 15U;
    uint32_t delay = (period * requested_phase + 180U) / 360U;
    uint32_t high_time = period / 5U;
    double actual_phase = 360.0 * delay / period;

    begin_pair(instance);
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, high_time),
                        "LESSONS", "Phase reference HIGH failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, period - high_time),
                        "LESSONS", "Phase reference LOW failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, delay),
                        "LESSONS", "Phase delay failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, high_time),
                        "LESSONS", "Phase shifted HIGH failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, period - delay - high_time),
                        "LESSONS", "Phase shifted LOW failed");

    uint32_t frequency = (1000000U + period / 2U) / period;
    instance->realized[0] = (scope_realized_channel_t){ frequency, period, high_time, 20 };
    instance->realized[1] = instance->realized[0];
    instance->parameters.phase_shift.delay_us = delay;
    instance->parameters.phase_shift.phase_degrees = actual_phase;
    snprintf(instance->context, sizeof(instance->context),
             "CH2 ist gegenüber der steigenden Referenzflanke auf CH1 verzögert.");
    set_numeric_measurement(instance, 0, "Periodendauer", SCOPE_MEASUREMENT_CHANNEL_1,
                            period, period * 0.02, "µs", 0, "Abstand zweier CH1-Flanken");
    set_numeric_measurement(instance, 1, "Zeitversatz", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                            delay, delay * 0.03, "µs", 0, "CH1-Anstieg bis CH2-Anstieg");
    set_numeric_measurement(instance, 2, "Phasenverschiebung", SCOPE_MEASUREMENT_DERIVED,
                            actual_phase, 2.0, "°", 1, "φ = 360° · Δt / T");
    return ESP_OK;
}

static esp_err_t generate_frequency_divider(scope_lesson_instance_t *instance)
{
    static const uint32_t periods[] = { 500, 1000, 2000 };
    static const uint32_t medium_ratios[] = { 2, 3, 4 };
    uint32_t input_period = choose(periods, 3);
    uint32_t ratio = instance->difficulty == 1 ? 2U :
                     instance->difficulty == 2 ? choose(medium_ratios, 3) : random_range(3, 8);
    uint32_t common_duration = input_period * ratio;
    uint32_t input_high = input_period / 2U;

    begin_pair(instance);
    for (uint32_t cycle = 0; cycle < ratio; ++cycle) {
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, input_high),
                            "LESSONS", "Divider input HIGH failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, input_period - input_high),
                            "LESSONS", "Divider input LOW failed");
    }
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, common_duration / 2U),
                        "LESSONS", "Divider output HIGH failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, common_duration - common_duration / 2U),
                        "LESSONS", "Divider output LOW failed");

    uint32_t input_frequency = (1000000U + input_period / 2U) / input_period;
    uint32_t output_frequency = (1000000U + common_duration / 2U) / common_duration;
    instance->realized[0] = (scope_realized_channel_t){ input_frequency, input_period, input_high, 50 };
    instance->realized[1] = (scope_realized_channel_t){ output_frequency, common_duration,
                                                        common_duration / 2U, 50 };
    instance->parameters.frequency_divider.ratio = ratio;
    snprintf(instance->context, sizeof(instance->context),
             "CH1 zeigt den Eingangstakt, CH2 den ganzzahlig geteilten Ausgangstakt.");
    set_numeric_measurement(instance, 0, "Eingangsfrequenz", SCOPE_MEASUREMENT_CHANNEL_1,
                            1000000.0 / input_period, input_frequency * 0.02, "Hz", 2, "fEin = 1 / TEin");
    set_numeric_measurement(instance, 1, "Ausgangsfrequenz", SCOPE_MEASUREMENT_CHANNEL_2,
                            1000000.0 / common_duration, output_frequency * 0.03, "Hz", 2, "fAus = 1 / TAus");
    set_numeric_measurement(instance, 2, "Teilverhältnis", SCOPE_MEASUREMENT_DERIVED,
                            ratio, 0, "zu 1", 0, "N = fEin / fAus");
    return ESP_OK;
}

static esp_err_t generate_ultrasonic(scope_lesson_instance_t *instance)
{
    static const uint32_t easy_distances[] = { 10, 20, 50, 100 };
    uint32_t distance = instance->difficulty == 1 ? choose(easy_distances, 4) :
                        instance->difficulty == 2 ? random_range(5, 200) : random_range(20, 300);
    uint32_t echo_width = distance * 58U;
    uint32_t echo_delay = instance->difficulty == 1 ? 500U :
                          instance->difficulty == 2 ? random_range(3, 10) * 100U :
                                                      random_range(200, 1200);
    uint32_t common_duration = 25000U;

    begin_pair(instance);
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, 10),
                        "LESSONS", "Ultrasonic trigger failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, common_duration - 10U),
                        "LESSONS", "Ultrasonic trigger idle failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, echo_delay),
                        "LESSONS", "Ultrasonic echo delay failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, echo_width),
                        "LESSONS", "Ultrasonic echo failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, common_duration - echo_delay - echo_width),
                        "LESSONS", "Ultrasonic echo idle failed");

    instance->realized[0] = (scope_realized_channel_t){ 40, common_duration, 10, 0 };
    instance->realized[1] = (scope_realized_channel_t){ 40, common_duration, echo_width, 0 };
    instance->parameters.ultrasonic.echo_delay_us = echo_delay;
    instance->parameters.ultrasonic.distance_cm = distance;
    snprintf(instance->context, sizeof(instance->context),
             "Simulierter Ultraschallsensor: Die Echo-HIGH-Zeit entspricht 58 µs je Zentimeter.");
    set_numeric_measurement(instance, 0, "Echo-Verzögerung", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                            echo_delay, echo_delay * 0.05, "µs", 0, "Trigger-Anstieg bis Echo-Anstieg");
    set_numeric_measurement(instance, 1, "Echo-Pulsbreite", SCOPE_MEASUREMENT_CHANNEL_2,
                            echo_width, echo_width * 0.03, "µs", 0, "HIGH-Zeit des Echo-Signals");
    set_numeric_measurement(instance, 2, "Entfernung", SCOPE_MEASUREMENT_DERIVED,
                            echo_width / 58.0, 2.0, "cm", 1, "s = tEcho / 58 µs/cm");
    return ESP_OK;
}

static esp_err_t generate_gated_pwm(scope_lesson_instance_t *instance)
{
    static const uint32_t periods[] = { 500, 1000, 2000 };
    uint32_t pwm_period = choose(periods, 3);
    uint32_t pwm_high = instance->difficulty == 1 ? pwm_period / 2U :
                        pwm_period * random_range(2, 8) / 10U;
    uint32_t pulse_count = instance->difficulty == 1 ? random_range(3, 5) :
                           instance->difficulty == 2 ? random_range(4, 8) : random_range(6, 10);
    uint32_t start_delay = instance->difficulty == 1 ? pwm_period :
                           instance->difficulty == 2 ? random_range(1, 3) * pwm_period / 2U :
                                                       random_range(200, 2000);
    uint32_t pre_idle = 3000U;
    uint32_t end_margin = 2000U;
    uint32_t gate_duration = start_delay + pulse_count * pwm_period + end_margin;
    uint32_t common_duration = pre_idle + gate_duration + 5000U;

    begin_pair(instance);
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, pre_idle),
                        "LESSONS", "Gate pre-idle failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, gate_duration),
                        "LESSONS", "Gate window failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, common_duration - pre_idle - gate_duration),
                        "LESSONS", "Gate post-idle failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, pre_idle + start_delay),
                        "LESSONS", "Gated PWM delay failed");
    for (uint32_t pulse = 0; pulse < pulse_count; ++pulse) {
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, pwm_high),
                            "LESSONS", "Gated PWM HIGH failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, pwm_period - pwm_high),
                            "LESSONS", "Gated PWM LOW failed");
    }
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false,
                        common_duration - pre_idle - start_delay - pulse_count * pwm_period),
                        "LESSONS", "Gated PWM post-idle failed");

    uint32_t pwm_frequency = (1000000U + pwm_period / 2U) / pwm_period;
    instance->realized[0] = (scope_realized_channel_t){ 0, common_duration, gate_duration, 0 };
    instance->realized[1] = (scope_realized_channel_t){ pwm_frequency, pwm_period, pwm_high,
                                                        (uint8_t)((100U * pwm_high) / pwm_period) };
    instance->parameters.gated_pwm.start_delay_us = start_delay;
    instance->parameters.gated_pwm.pulse_count = pulse_count;
    snprintf(instance->context, sizeof(instance->context),
             "CH1 ist das Freigabefenster; nur innerhalb dieses Fensters erscheint die PWM auf CH2.");
    set_numeric_measurement(instance, 0, "PWM-Startverzögerung", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                            start_delay, start_delay * 0.04, "µs", 0, "Gate-Anstieg bis erster PWM-Anstieg");
    set_numeric_measurement(instance, 1, "Pulse im Fenster", SCOPE_MEASUREMENT_CHANNEL_2,
                            pulse_count, 0, "", 0, "Vollständige HIGH-Pulse zählen");
    set_numeric_measurement(instance, 2, "Freigabedauer", SCOPE_MEASUREMENT_CHANNEL_1,
                            gate_duration / 1000.0, gate_duration * 0.00003,
                            "ms", 3, "HIGH-Zeit des Freigabefensters");
    return ESP_OK;
}

static esp_err_t generate_quadrature(scope_lesson_instance_t *instance)
{
    static const uint32_t periods[] = { 1000, 2000, 4000 };
    uint32_t period = choose(periods, 3);
    uint32_t quarter = period / 4U;
    bool channel_a_leads = instance->difficulty == 1 ? true : (esp_random() & 1U) != 0;

    begin_pair(instance);
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, true, period / 2U),
                        "LESSONS", "Quadrature A HIGH failed");
    ESP_RETURN_ON_ERROR(append_pair_segment(instance, 0, false, period / 2U),
                        "LESSONS", "Quadrature A LOW failed");
    if (channel_a_leads) {
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, quarter),
                            "LESSONS", "Quadrature B delay failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, period / 2U),
                            "LESSONS", "Quadrature B HIGH failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, quarter),
                            "LESSONS", "Quadrature B LOW failed");
    } else {
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, quarter),
                            "LESSONS", "Quadrature B leading HIGH failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, false, period / 2U),
                            "LESSONS", "Quadrature B LOW failed");
        ESP_RETURN_ON_ERROR(append_pair_segment(instance, 1, true, quarter),
                            "LESSONS", "Quadrature B wrap HIGH failed");
    }

    uint32_t frequency = (1000000U + period / 2U) / period;
    instance->realized[0] = (scope_realized_channel_t){ frequency, period, period / 2U, 50 };
    instance->realized[1] = instance->realized[0];
    instance->parameters.quadrature.channel_a_leads = channel_a_leads;
    instance->parameters.quadrature.quarter_period_us = quarter;
    snprintf(instance->context, sizeof(instance->context),
             "Spur A liegt auf CH1, Spur B auf CH2. Die führende Spur bestimmt die Richtung.");
    set_numeric_measurement(instance, 0, "Periodendauer", SCOPE_MEASUREMENT_CHANNEL_1,
                            period, period * 0.02, "µs", 0, "Abstand gleicher Flanken auf Spur A");
    set_numeric_measurement(instance, 1, "Flankenversatz", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                            quarter, quarter * 0.03, "µs", 0, "Viertelperiode zwischen Spur A und B");
    set_text_measurement(instance, 2, "Führende Spur", SCOPE_MEASUREMENT_CHANNEL_PAIR,
                         channel_a_leads ? "A führt B" : "B führt A",
                         "Die zuerst wechselnde Spur gibt die simulierte Richtung vor");
    return ESP_OK;
}

static uint32_t signature_mix(uint32_t hash, uint32_t value)
{
    hash ^= value;
    return hash * 16777619U;
}

static uint32_t calculate_signature(const scope_lesson_instance_t *instance)
{
    uint32_t hash = 2166136261U;
    hash = signature_mix(hash, instance->lesson->id);
    hash = signature_mix(hash, instance->signal.kind);
    for (size_t channel = 0; channel < SCOPEBUDDY_MAX_CHANNELS; ++channel) {
        hash = signature_mix(hash, instance->realized[channel].frequency_hz);
        hash = signature_mix(hash, instance->realized[channel].period_us);
        hash = signature_mix(hash, instance->realized[channel].high_time_us);
        hash = signature_mix(hash, instance->realized[channel].duty_percent);
    }
    if (instance->signal.kind == SCOPE_SIGNAL_SEQUENCE) {
        const scope_timeline_spec_t *timeline = &instance->signal.data.sequence.timeline;
        hash = signature_mix(hash, timeline->segment_count);
        for (size_t i = 0; i < timeline->segment_count; ++i) {
            hash = signature_mix(hash, timeline->segments[i].level);
            hash = signature_mix(hash, timeline->segments[i].duration_us);
        }
        hash = signature_mix(hash, instance->signal.data.sequence.loop);
    } else if (instance->signal.kind == SCOPE_SIGNAL_SEQUENCE_PAIR) {
        for (size_t channel = 0; channel < SCOPEBUDDY_MAX_CHANNELS; ++channel) {
            const scope_timeline_spec_t *timeline = &instance->signal.data.pair.channels[channel];
            hash = signature_mix(hash, timeline->segment_count);
            for (size_t i = 0; i < timeline->segment_count; ++i) {
                hash = signature_mix(hash, timeline->segments[i].level);
                hash = signature_mix(hash, timeline->segments[i].duration_us);
            }
        }
        hash = signature_mix(hash, instance->signal.data.pair.loop);
    } else if (instance->signal.kind == SCOPE_SIGNAL_ALTERNATING) {
        hash = signature_mix(hash, instance->signal.data.alternating.state_duration_ms);
    }
    return hash;
}

size_t scopebuddy_lesson_count(void)
{
    return SCOPEBUDDY_LESSON_COUNT;
}

const scope_lesson_definition_t *scopebuddy_lesson_at(size_t index)
{
    return index < scopebuddy_lesson_count() ? &lessons[index] : NULL;
}

esp_err_t scopebuddy_generate_lesson(scope_lesson_id_t id, uint8_t difficulty,
                                     scope_lesson_instance_t *instance)
{
    if (instance == NULL || (size_t)id >= scopebuddy_lesson_count() || difficulty < 1 || difficulty > 3) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(instance, 0, sizeof(*instance));
    instance->lesson = &lessons[id];
    instance->difficulty = difficulty;
    instance->measurement_count = SCOPEBUDDY_MAX_MEASUREMENTS;
    esp_err_t err;
    switch (id) {
    case SCOPE_LESSON_PERIODIC:
    case SCOPE_LESSON_PULSE_WIDTH: err = generate_pwm(instance); break;
    case SCOPE_LESSON_BURST: err = generate_burst(instance); break;
    case SCOPE_LESSON_MISSING_PULSE: err = generate_missing_pulse(instance); break;
    case SCOPE_LESSON_SERVO: err = generate_servo(instance); break;
    case SCOPE_LESSON_TACHOMETER: err = generate_tachometer(instance); break;
    case SCOPE_LESSON_BUTTON_BOUNCE: err = generate_button_bounce(instance); break;
    case SCOPE_LESSON_UART: err = generate_uart(instance); break;
    case SCOPE_LESSON_ALTERNATING: err = generate_alternating(instance); break;
    case SCOPE_LESSON_TRIGGER_RESPONSE: err = generate_trigger_response(instance); break;
    case SCOPE_LESSON_PHASE_SHIFT: err = generate_phase_shift(instance); break;
    case SCOPE_LESSON_FREQUENCY_DIVIDER: err = generate_frequency_divider(instance); break;
    case SCOPE_LESSON_ULTRASONIC: err = generate_ultrasonic(instance); break;
    case SCOPE_LESSON_GATED_PWM: err = generate_gated_pwm(instance); break;
    case SCOPE_LESSON_QUADRATURE: err = generate_quadrature(instance); break;
    default: err = ESP_ERR_INVALID_ARG; break;
    }
    if (err == ESP_OK) instance->signature = calculate_signature(instance);
    return err;
}

void scopebuddy_update_effective_pwm(scope_lesson_instance_t *instance,
                                     uint32_t frequency_a_hz, uint8_t duty_a_percent,
                                     uint32_t frequency_b_hz, uint8_t duty_b_percent)
{
    if (instance == NULL) return;
    set_realized_pwm(&instance->realized[0], frequency_a_hz, duty_a_percent);
    if (frequency_b_hz > 0) set_realized_pwm(&instance->realized[1], frequency_b_hz, duty_b_percent);
    format_pwm_measurements(instance);
    instance->signature = calculate_signature(instance);
}
