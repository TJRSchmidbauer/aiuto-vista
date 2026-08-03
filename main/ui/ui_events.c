#include "ui.h"
#include "bsp_extra.h"
#include "encoder_input.h"
#include "scopebuddy_lessons.h"
#include "scopebuddy_output.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>

#define SCOPEBUDDY_FIRMWARE_VERSION "0.4"
#define UI_TAG "SCOPEBUDDY_UI"
#define SETTINGS_NAMESPACE "scopebuddy"
#define SETTINGS_KEY "ui_flags"
#define SETTING_FLAG_TIMER       (1U << 0)
#define SETTING_FLAG_VALUES      (1U << 1)
#define SETTING_FLAG_SCOPE_RESET (1U << 2)
#define SETTING_FLAG_ENCODER     (1U << 3)
#define LESSONS_PER_PAGE         3U

LV_FONT_DECLARE(scopebuddy_font_14);
LV_FONT_DECLARE(scopebuddy_font_24);

static bool hardware_ready;
static bool alternating_state_b;
static uint8_t question_number;
static bool lesson_selected;
static scope_lesson_id_t selected_lesson;
static scope_lesson_instance_t challenge;
static uint8_t lesson_page;
static esp_timer_handle_t pattern_timer;
static StaticSemaphore_t pattern_mutex_buffer;
static SemaphoreHandle_t pattern_mutex;
static bool pattern_enabled;
static uint32_t recent_signatures[18];
static uint8_t recent_count;

static size_t lesson_page_count(void)
{
    return (scopebuddy_lesson_count() + LESSONS_PER_PAGE - 1U) / LESSONS_PER_PAGE;
}

static lv_obj_t *action_button;
static lv_obj_t *action_label;
static lv_obj_t *all_values_button;
static lv_obj_t *all_values_label;
static lv_obj_t *advance_button;
static lv_obj_t *timer_label;
static lv_timer_t *game_clock;
static lv_timer_t *splash_timer;
static lv_timer_t *diagnostics_timer;
static lv_obj_t *diagnostics_values_label;
static lv_obj_t *sync_test_status_label;
static lv_obj_t *sync_test_action_label;
static bool sync_test_running;
static uint32_t elapsed_seconds;
static lv_obj_t *measurement_values[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *measurement_boxes[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *measurement_marks[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *confirm_overlay;
static lv_group_t *confirm_group;
static lv_group_t *confirm_background_group;
static lv_obj_t *page_default_focus;
static bool measurement_selected[SCOPEBUDDY_MAX_MEASUREMENTS];
static bool measurement_revealed[SCOPEBUDDY_MAX_MEASUREMENTS];
static bool setting_show_timer = true;
static bool setting_reveal_values;
static bool setting_scope_reset = true;
static bool setting_encoder_enabled = true;
static bool settings_loaded;
static bool splash_active;

static void mode_event(lv_event_t *event);
static void page_event(lv_event_t *event);
static void build_splash_screen(void);
static void build_start_screen(void);
static void build_settings_screen(void);
static void build_diagnostics_screen(void);
static void build_sync_test_screen(void);
static void build_scope_reset_screen(void);
static void build_question_screen(void);
static void reveal_measurement(uint8_t index, const char *value);
static void log_operation_error(const char *operation, esp_err_t err);

static uint8_t settings_flags(void)
{
    return (setting_show_timer ? SETTING_FLAG_TIMER : 0U) |
           (setting_reveal_values ? SETTING_FLAG_VALUES : 0U) |
           (setting_scope_reset ? SETTING_FLAG_SCOPE_RESET : 0U) |
           (setting_encoder_enabled ? SETTING_FLAG_ENCODER : 0U);
}

static void load_settings(void)
{
    if (settings_loaded) return;
    settings_loaded = true;

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(UI_TAG, "No stored settings; using defaults");
        encoder_input_set_enabled(setting_encoder_enabled);
        return;
    }
    if (err != ESP_OK) {
        log_operation_error("Opening stored settings", err);
        encoder_input_set_enabled(setting_encoder_enabled);
        return;
    }

    uint8_t flags = 0;
    esp_err_t flags_err = nvs_get_u8(handle, SETTINGS_KEY, &flags);
    if (flags_err == ESP_OK) {
        setting_show_timer = (flags & SETTING_FLAG_TIMER) != 0;
        setting_reveal_values = (flags & SETTING_FLAG_VALUES) != 0;
        setting_scope_reset = (flags & SETTING_FLAG_SCOPE_RESET) != 0;
        setting_encoder_enabled = (flags & SETTING_FLAG_ENCODER) != 0;
        ESP_LOGI(UI_TAG, "Loaded settings flags: 0x%02x", flags);
    } else if (flags_err != ESP_ERR_NVS_NOT_FOUND) {
        log_operation_error("Reading stored settings", flags_err);
    }

    nvs_close(handle);
    encoder_input_set_enabled(setting_encoder_enabled);
}

static void save_settings(void)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) err = nvs_set_u8(handle, SETTINGS_KEY, settings_flags());
    if (err == ESP_OK) err = nvs_commit(handle);
    if (handle) nvs_close(handle);
    log_operation_error("Saving settings", err);
}

static void log_ui_memory(const char *stage)
{
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    ESP_LOGI(UI_TAG,
             "LVGL memory [%s]: free=%lu, largest=%lu, used=%u%%, fragmentation=%u%%, max_used=%lu",
             stage,
             (unsigned long)memory.free_size,
             (unsigned long)memory.free_biggest_size,
             memory.used_pct,
             memory.frag_pct,
             (unsigned long)memory.max_used);
}

static void log_operation_error(const char *operation, esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(UI_TAG, "%s failed: %s", operation, esp_err_to_name(err));
    }
}

static void clock_callback(lv_timer_t *timer)
{
    (void)timer;
    if (!lesson_selected || !timer_label) return;
    ++elapsed_seconds;
    lv_label_set_text_fmt(timer_label, "%02lu:%02lu",
                          (unsigned long)(elapsed_seconds / 60U),
                          (unsigned long)(elapsed_seconds % 60U));
}

static const char *mode_name(void)
{
    return challenge.lesson ? challenge.lesson->title : "LEKTION";
}

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
    case ESP_RST_POWERON: return "POWER-ON";
    case ESP_RST_EXT: return "EXTERNER RESET";
    case ESP_RST_SW: return "SOFTWARE-RESET";
    case ESP_RST_PANIC: return "PANIC/ASSERT";
    case ESP_RST_INT_WDT: return "INTERRUPT-WATCHDOG";
    case ESP_RST_TASK_WDT: return "TASK-WATCHDOG";
    case ESP_RST_WDT: return "WATCHDOG";
    case ESP_RST_DEEPSLEEP: return "DEEP-SLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    case ESP_RST_USB: return "USB";
    case ESP_RST_JTAG: return "JTAG";
    case ESP_RST_EFUSE: return "EFUSE-FEHLER";
    case ESP_RST_PWR_GLITCH: return "POWER-GLITCH";
    case ESP_RST_CPU_LOCKUP: return "CPU-LOCKUP";
    default: return "UNBEKANNT";
    }
}

static uint32_t challenge_signature(void)
{
    return challenge.signature;
}

static bool signature_seen(uint32_t signature)
{
    for (uint8_t i = 0; i < recent_count; ++i) {
        if (recent_signatures[i] == signature) return true;
    }
    return false;
}

static void remember_signature(uint32_t signature)
{
    if (recent_count < (sizeof(recent_signatures) / sizeof(recent_signatures[0]))) {
        recent_signatures[recent_count++] = signature;
        return;
    }
    for (uint8_t i = 1; i < recent_count; ++i) recent_signatures[i - 1] = recent_signatures[i];
    recent_signatures[recent_count - 1] = signature;
}

static void stop_pattern_locked(void)
{
    pattern_enabled = false;
    if (pattern_timer && esp_timer_is_active(pattern_timer)) {
        esp_err_t err = esp_timer_stop(pattern_timer);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            log_operation_error("Stopping signal timer", err);
        }
    }
    log_operation_error("Stopping challenge output", scopebuddy_output_stop());
}

static void stop_pattern(void)
{
    if (pattern_mutex == NULL) {
        log_operation_error("Stopping challenge output", scopebuddy_output_stop());
        return;
    }
    xSemaphoreTake(pattern_mutex, portMAX_DELAY);
    stop_pattern_locked();
    xSemaphoreGive(pattern_mutex);
}

static bool schedule_pattern_locked(uint64_t delay_us)
{
    if (pattern_timer == NULL || !pattern_enabled) return false;
    esp_err_t err = esp_timer_start_once(pattern_timer, delay_us);
    if (err != ESP_OK) {
        pattern_enabled = false;
        log_operation_error("Starting signal timer", err);
        return false;
    }
    return true;
}

static void pattern_callback(void *arg)
{
    (void)arg;
    if (pattern_mutex == NULL) return;
    xSemaphoreTake(pattern_mutex, portMAX_DELAY);
    if (!pattern_enabled) {
        xSemaphoreGive(pattern_mutex);
        return;
    }

    if (challenge.signal.kind == SCOPE_SIGNAL_ALTERNATING) {
        const scope_alternating_spec_t *alternating = &challenge.signal.data.alternating;
        alternating_state_b = !alternating_state_b;
        esp_err_t err = gpio_wave_set_frequency(
            alternating_state_b ? alternating->state_b.frequency_hz : alternating->state_a.frequency_hz);
        if (err == ESP_OK) {
            err = gpio_wave_set_duty(alternating_state_b ? alternating->state_b.duty_percent :
                                                            alternating->state_a.duty_percent);
        }
        if (err == ESP_OK) {
            schedule_pattern_locked((uint64_t)alternating->state_duration_ms * 1000ULL);
        } else {
            pattern_enabled = false;
            log_operation_error("Switching alternating output", err);
        }
    }
    xSemaphoreGive(pattern_mutex);
}

static void ensure_pattern_timer(void)
{
    if (pattern_timer) return;
    if (pattern_mutex == NULL) {
        pattern_mutex = xSemaphoreCreateMutexStatic(&pattern_mutex_buffer);
        if (pattern_mutex == NULL) {
            ESP_LOGE(UI_TAG, "Creating signal mutex failed");
            return;
        }
    }
    const esp_timer_create_args_t args = {
        .callback = pattern_callback,
        .name = "measurement_game",
    };
    esp_err_t err = esp_timer_create(&args, &pattern_timer);
    if (err != ESP_OK) {
        pattern_timer = NULL;
        log_operation_error("Creating signal timer", err);
    }
}

static void generate_challenge(void)
{
    esp_err_t err;
    uint8_t attempts = 0;
    do {
        err = scopebuddy_generate_lesson(selected_lesson, question_number, &challenge);
        if (err != ESP_OK) {
            log_operation_error("Generating lesson", err);
            return;
        }
        ++attempts;
    } while (signature_seen(challenge_signature()) && attempts < 20U);
    remember_signature(challenge_signature());
}

static void start_challenge_signal(void)
{
    if (pattern_mutex == NULL) {
        ESP_LOGE(UI_TAG, "Cannot start challenge: signal mutex unavailable");
        return;
    }
    xSemaphoreTake(pattern_mutex, portMAX_DELAY);
    stop_pattern_locked();

    esp_err_t err = scopebuddy_output_start(&challenge.signal);
    if (err != ESP_OK) {
        log_operation_error("Starting challenge output", err);
        xSemaphoreGive(pattern_mutex);
        return;
    }

    if (challenge.signal.kind == SCOPE_SIGNAL_PWM) {
        uint32_t actual_frequency = 0;
        uint8_t actual_duty = 0;
        if (gpio_wave_get_effective(&actual_frequency, &actual_duty) == ESP_OK) {
            scopebuddy_update_effective_pwm(&challenge, actual_frequency, actual_duty, 0, 0);
        }
    } else if (challenge.signal.kind == SCOPE_SIGNAL_ALTERNATING) {
        const scope_alternating_spec_t *alternating = &challenge.signal.data.alternating;
        uint32_t actual_a = 0;
        uint32_t actual_b = 0;
        uint8_t duty_a = 0;
        uint8_t duty_b = 0;
        if (gpio_wave_get_effective(&actual_a, &duty_a) == ESP_OK &&
            gpio_wave_set_frequency(alternating->state_b.frequency_hz) == ESP_OK &&
            gpio_wave_set_duty(alternating->state_b.duty_percent) == ESP_OK &&
            gpio_wave_get_effective(&actual_b, &duty_b) == ESP_OK &&
            gpio_wave_set_frequency(alternating->state_a.frequency_hz) == ESP_OK &&
            gpio_wave_set_duty(alternating->state_a.duty_percent) == ESP_OK) {
            scopebuddy_update_effective_pwm(&challenge, actual_a, duty_a, actual_b, duty_b);
        }
        pattern_enabled = true;
        alternating_state_b = false;
        schedule_pattern_locked((uint64_t)alternating->state_duration_ms * 1000ULL);
    }
    xSemaphoreGive(pattern_mutex);
}

static void start_scope_reset_signal(void)
{
    if (pattern_mutex == NULL) {
        ESP_LOGE(UI_TAG, "Cannot start reset signal: signal mutex unavailable");
        return;
    }
    xSemaphoreTake(pattern_mutex, portMAX_DELAY);
    stop_pattern_locked();

    if (challenge.lesson && challenge.lesson->required_channels == 2) {
        scope_signal_spec_t reset_signal = { .kind = SCOPE_SIGNAL_SEQUENCE_PAIR };
        reset_signal.data.pair.loop = true;
        reset_signal.data.pair.channels[0].segments[0] =
            (scope_signal_segment_t){ .level = true, .duration_us = 500 };
        reset_signal.data.pair.channels[0].segments[1] =
            (scope_signal_segment_t){ .level = false, .duration_us = 500 };
        reset_signal.data.pair.channels[0].segment_count = 2;
        reset_signal.data.pair.channels[1].segments[0] =
            (scope_signal_segment_t){ .level = false, .duration_us = 250 };
        reset_signal.data.pair.channels[1].segments[1] =
            (scope_signal_segment_t){ .level = true, .duration_us = 500 };
        reset_signal.data.pair.channels[1].segments[2] =
            (scope_signal_segment_t){ .level = false, .duration_us = 250 };
        reset_signal.data.pair.channels[1].segment_count = 3;
        esp_err_t pair_err = scopebuddy_output_start(&reset_signal);
        log_operation_error("Starting two-channel oscilloscope reset output", pair_err);
        xSemaphoreGive(pattern_mutex);
        return;
    }

    /* Use the safe extremes supported by the GPIO waveform driver. The choice
       is based on the signal state shown first in the upcoming challenge so
       AUTO leaves the horizontal timebase as far away from it as possible. */
    uint32_t challenge_frequency = challenge.realized[0].frequency_hz;
    if (challenge_frequency == 0 && challenge.realized[0].period_us > 0) {
        challenge_frequency = 1000000U / challenge.realized[0].period_us;
    }
    uint32_t reset_frequency = (challenge_frequency < 448U) ? 20000U : 10U;
    esp_err_t err = gpio_wave_set_frequency(reset_frequency);
    if (err == ESP_OK) err = gpio_wave_set_duty(50);
    if (err == ESP_OK) err = gpio_wave_start();
    log_operation_error("Starting oscilloscope reset output", err);
    xSemaphoreGive(pattern_mutex);
}

static void prepare_question(void)
{
    /* Stop the previous timer before replacing the challenge data it reads. */
    stop_pattern();
    generate_challenge();
    if (setting_scope_reset) build_scope_reset_screen();
    else build_question_screen();
}

static void prepare_question_async(void *data)
{
    (void)data;
    prepare_question();
}

static void build_question_async(void *data)
{
    (void)data;
    build_question_screen();
}

static void build_settings_async(void *data)
{
    (void)data;
    build_settings_screen();
}

static void build_diagnostics_async(void *data)
{
    (void)data;
    build_diagnostics_screen();
}

static void build_sync_test_async(void *data)
{
    (void)data;
    build_sync_test_screen();
}

static void build_start_async(void *data)
{
    (void)data;
    build_start_screen();
}

static void queue_ui_action(lv_async_cb_t callback, const char *name)
{
    if (lv_async_call(callback, NULL) != LV_RES_OK) {
        ESP_LOGE(UI_TAG, "Queuing UI action '%s' failed", name);
    }
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int x, int y,
                            const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label,
                               font == &lv_font_montserrat_24 ? &scopebuddy_font_24 : &scopebuddy_font_14,
                               0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int x, int y,
                             int width, int height, uint32_t color,
                             lv_event_cb_t callback, void *user_data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_RELEASED, user_data);
    lv_obj_add_event_cb(button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &scopebuddy_font_14, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);
    return button;
}

static void make_divider(lv_obj_t *parent, int x, int y, int width)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, width, 2);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
}

static void splash_set_opacity(void *object, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)object, (lv_opa_t)value, 0);
}

static void splash_set_zoom(void *object, int32_t value)
{
    lv_img_set_zoom((lv_obj_t *)object, (uint16_t)value);
}

static void splash_finish_callback(lv_timer_t *timer)
{
    (void)timer;
    splash_timer = NULL;
    splash_active = false;
    build_start_screen();
}

static void build_splash_screen(void)
{
    log_ui_memory("before splash cleanup");
    lv_obj_clean(ui_Screen1);
    lesson_selected = false;
    timer_label = NULL;
    splash_active = true;

    lv_obj_t *background = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(background, 0, 0);
    lv_obj_set_size(background, 800, 480);
    lv_obj_clear_flag(background, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(background, 0, 0);
    lv_obj_set_style_bg_color(background, lv_color_hex(0x050B12), 0);
    lv_obj_set_style_bg_opa(background, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(background, 0, 0);
    lv_obj_set_style_pad_all(background, 0, 0);

    lv_obj_t *title = lv_label_create(background);
    lv_label_set_text(title, "ScopeBuddy");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF4FAFF), 0);
    lv_obj_update_layout(background);

    const int logo_width = 80;
    const int logo_height = 139;
    const int brand_gap = 22;
    const int brand_y = 142;
    int brand_width = logo_width + brand_gap + lv_obj_get_width(title);
    int brand_x = (800 - brand_width) / 2;

    lv_obj_t *logo = lv_img_create(background);
    lv_img_set_src(logo, &ui_img_scopebuddy_splash);
    lv_obj_set_pos(logo, brand_x, brand_y);
    lv_img_set_pivot(logo, 40, 70);
    lv_img_set_zoom(logo, 150);
    lv_obj_set_style_opa(logo, LV_OPA_TRANSP, 0);

    lv_obj_set_pos(title, brand_x + logo_width + brand_gap,
                   brand_y + (logo_height - lv_obj_get_height(title)) / 2);
    lv_obj_set_style_opa(title, LV_OPA_TRANSP, 0);

    lv_obj_t *version = make_label(background,
                                   "Firmware-Version " SCOPEBUDDY_FIRMWARE_VERSION,
                                   0, 350, &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 350);
    lv_obj_set_style_opa(version, LV_OPA_TRANSP, 0);

    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, logo);
    lv_anim_set_exec_cb(&animation, splash_set_zoom);
    lv_anim_set_values(&animation, 150, 256);
    lv_anim_set_time(&animation, 650);
    lv_anim_set_delay(&animation, 100);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, logo);
    lv_anim_set_exec_cb(&animation, splash_set_opacity);
    lv_anim_set_values(&animation, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&animation, 480);
    lv_anim_set_delay(&animation, 100);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, title);
    lv_anim_set_exec_cb(&animation, splash_set_opacity);
    lv_anim_set_values(&animation, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&animation, 420);
    lv_anim_set_delay(&animation, 620);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, version);
    lv_anim_set_exec_cb(&animation, splash_set_opacity);
    lv_anim_set_values(&animation, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&animation, 320);
    lv_anim_set_delay(&animation, 900);
    lv_anim_start(&animation);

    splash_timer = lv_timer_create(splash_finish_callback, 2800, NULL);
    if (splash_timer) {
        lv_timer_set_repeat_count(splash_timer, 1);
    } else {
        ESP_LOGE(UI_TAG, "Creating splash timer failed");
    }
    log_ui_memory("splash ready");
}

static void update_solution_buttons(void)
{
    if (!action_button || !action_label || !all_values_label) return;

    bool any_selected = false;
    bool all_selected_visible = true;
    bool all_visible = true;
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (measurement_selected[i]) {
            any_selected = true;
            if (!measurement_revealed[i]) all_selected_visible = false;
        }
        if (!measurement_revealed[i]) all_visible = false;
    }

    if (any_selected) lv_obj_clear_state(action_button, LV_STATE_DISABLED);
    else lv_obj_add_state(action_button, LV_STATE_DISABLED);
    lv_obj_set_style_text_color(action_label,
                                lv_color_hex(any_selected ? 0xDCE8F7 : 0x607895), 0);
    lv_label_set_text(action_label,
                      any_selected && all_selected_visible ?
                      "AUSGEWÄHLTE WERTE\nVERBERGEN" :
                      "AUSGEWÄHLTE WERTE\nANZEIGEN");
    lv_label_set_text(all_values_label,
                      all_visible ? "ALLE WERTE\nVERBERGEN" : "ALLE WERTE\nANZEIGEN");
}

static void measurement_event(lv_event_t *event)
{
    uint8_t index = (uint8_t)(uintptr_t)lv_event_get_user_data(event);
    measurement_selected[index] = !measurement_selected[index];
    lv_label_set_text(measurement_marks[index], measurement_selected[index] ? "X" : "");
    lv_obj_set_style_bg_color(measurement_boxes[index], lv_color_hex(0x1455B8), 0);
    lv_obj_set_style_bg_opa(measurement_boxes[index],
                            measurement_selected[index] ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

    update_solution_buttons();
}

static const char *measurement_source_name(scope_measurement_source_t source)
{
    switch (source) {
    case SCOPE_MEASUREMENT_CHANNEL_1: return "CH1";
    case SCOPE_MEASUREMENT_CHANNEL_2: return "CH2";
    case SCOPE_MEASUREMENT_CHANNEL_PAIR: return "CH1/CH2";
    case SCOPE_MEASUREMENT_DERIVED: return "BERECHNET";
    default: return "";
    }
}

static uint32_t measurement_source_color(scope_measurement_source_t source)
{
    switch (source) {
    case SCOPE_MEASUREMENT_CHANNEL_1: return 0x2684FF;
    case SCOPE_MEASUREMENT_CHANNEL_2: return 0xE6B43C;
    case SCOPE_MEASUREMENT_CHANNEL_PAIR: return 0x18B8C9;
    case SCOPE_MEASUREMENT_DERIVED: return 0x9A70E5;
    default: return 0x607895;
    }
}

static void make_measurement_item(lv_obj_t *parent, const scope_measurement_t *measurement,
                                  int y, uint8_t index)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, 25, y - 3);
    lv_obj_set_size(box, 30, 30);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(box, 5, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    measurement_boxes[index] = box;
    measurement_marks[index] = make_label(box, "", 8, 4, &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_add_event_cb(box, measurement_event, LV_EVENT_RELEASED,
                        (void *)(uintptr_t)index);
    lv_obj_add_event_cb(box, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_group_t *encoder_group = lv_group_get_default();
    if (encoder_group) lv_group_add_obj(encoder_group, box);
    make_label(parent, measurement_source_name(measurement->source), 70, y + 3,
               &lv_font_montserrat_14, measurement_source_color(measurement->source));
    lv_obj_t *measurement_label = make_label(parent, measurement->label, 153, y + 3,
                                              &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_set_width(measurement_label, 205);
    measurement_values[index] = make_label(parent, "---", 370, y + 3,
                                            &lv_font_montserrat_14, 0x607895);

    lv_obj_t *touch_area = lv_obj_create(parent);
    lv_obj_set_pos(touch_area, 18, y - 5);
    lv_obj_set_size(touch_area, 500, 35);
    lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(touch_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(touch_area, 0, 0);
    lv_obj_set_style_pad_all(touch_area, 0, 0);
    lv_obj_add_event_cb(touch_area, measurement_event, LV_EVENT_RELEASED,
                        (void *)(uintptr_t)index);
    lv_obj_add_event_cb(touch_area, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
}

static void reveal_measurement(uint8_t index, const char *value)
{
    lv_label_set_text(measurement_values[index], value);
    lv_obj_set_style_text_color(measurement_values[index], lv_color_hex(0x5DE08B), 0);
    measurement_revealed[index] = true;
}

static void hide_measurement(uint8_t index)
{
    lv_label_set_text(measurement_values[index], "---");
    lv_obj_set_style_text_color(measurement_values[index], lv_color_hex(0x607895), 0);
    measurement_revealed[index] = false;
}

static void apply_direct_value_setting(void)
{
    if (!setting_reveal_values) return;
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (measurement_values[i]) reveal_measurement(i, challenge.measurements[i].value);
    }
    if (all_values_button) lv_obj_add_flag(all_values_button, LV_OBJ_FLAG_HIDDEN);
    if (action_button) lv_obj_add_flag(action_button, LV_OBJ_FLAG_HIDDEN);
}

static void make_mode_card(int x, const scope_lesson_definition_t *lesson)
{
    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, x, 122);
    lv_obj_set_size(card, 230, 278);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    lv_obj_set_style_border_color(card, lv_color_hex(lesson->accent), 0);
    lv_obj_t *title = make_label(card, lesson->title, 18, 20,
                                 &lv_font_montserrat_24, lesson->accent);
    lv_obj_set_width(title, 198);
    lv_obj_set_style_text_font(title, &scopebuddy_font_14, 0);
    make_label(card, lesson->category, 18, 58, &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_t *description = make_label(card, lesson->summary, 18, 98,
                                       &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_width(description, 194);
    lv_obj_set_style_text_line_space(description, 6, 0);
    lv_obj_t *status = make_label(card, "3 STUFEN", 18, 190,
                                  &lv_font_montserrat_14, 0x607895);
    lv_obj_set_width(status, 140);
    char channel_badge[8];
    snprintf(channel_badge, sizeof(channel_badge), "%u CH", lesson->required_channels);
    lv_obj_t *badge = make_label(card, channel_badge, 160, 190,
                                 &lv_font_montserrat_14,
                                 lesson->required_channels == 2 ? 0x18B8C9 : 0x607895);
    lv_obj_set_width(badge, 52);
    lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_RIGHT, 0);
    make_button(card, "STARTEN", 18, 222, 194, 42, lesson->accent,
                mode_event, (void *)(uintptr_t)lesson->id);
}

static void mode_event(lv_event_t *event)
{
    selected_lesson = (scope_lesson_id_t)(uintptr_t)lv_event_get_user_data(event);
    const scope_lesson_definition_t *lesson = scopebuddy_lesson_at(selected_lesson);
    if (lesson == NULL) return;
    lesson_selected = true;
    question_number = 1;
    queue_ui_action(prepare_question_async, "prepare question");
}

static void page_event(lv_event_t *event)
{
    intptr_t direction = (intptr_t)lv_event_get_user_data(event);
    size_t page_count = lesson_page_count();
    if (direction < 0 && lesson_page > 0) --lesson_page;
    if (direction > 0 && (size_t)lesson_page + 1U < page_count) ++lesson_page;
    queue_ui_action(build_start_async, "lesson page");
}

static void reveal_selected_value(uint8_t index)
{
    if (index >= challenge.measurement_count) return;
    reveal_measurement(index, challenge.measurements[index].value);
}

static void solve_event(lv_event_t *event)
{
    (void)event;
    bool hide_selected = true;
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (measurement_selected[i] && !measurement_revealed[i]) hide_selected = false;
    }
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (!measurement_selected[i]) continue;
        if (hide_selected) hide_measurement(i);
        else reveal_selected_value(i);
    }
    update_solution_buttons();
}

static void solve_all_event(lv_event_t *event)
{
    (void)event;
    bool hide_all = true;
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (!measurement_revealed[i]) hide_all = false;
    }
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        if (hide_all) hide_measurement(i);
        else reveal_selected_value(i);
    }
    update_solution_buttons();
}

static void advance_event(lv_event_t *event)
{
    (void)event;
    if (question_number >= 3) {
        stop_pattern();
        queue_ui_action(build_start_async, "start screen");
    } else {
        ++question_number;
        queue_ui_action(prepare_question_async, "next question");
    }
}

static void begin_question_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_question_async, "question screen");
}

static void return_home_async(void *data)
{
    (void)data;
    confirm_overlay = NULL;
    build_start_screen();
}

static void close_confirm_encoder_group(void)
{
    if (!confirm_group) return;

    encoder_input_activate_group(confirm_background_group);
    lv_group_del(confirm_group);
    confirm_group = NULL;
    confirm_background_group = NULL;
}

static void home_dialog_event(lv_event_t *event)
{
    bool return_home = (bool)(uintptr_t)lv_event_get_user_data(event);
    close_confirm_encoder_group();
    if (return_home) {
        stop_pattern();
        queue_ui_action(return_home_async, "return home");
    } else {
        lv_obj_del_async(confirm_overlay);
        confirm_overlay = NULL;
        if (page_default_focus && lv_obj_is_valid(page_default_focus)) {
            lv_group_focus_obj(page_default_focus);
        }
    }
}

static void home_event(lv_event_t *event)
{
    (void)event;
    if (confirm_overlay) return;

    confirm_background_group = lv_group_get_default();
    if (confirm_background_group) {
        confirm_group = lv_group_create();
        if (confirm_group) lv_group_set_default(confirm_group);
    }

    confirm_overlay = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(confirm_overlay, 0, 0);
    lv_obj_set_size(confirm_overlay, 800, 480);
    lv_obj_clear_flag(confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(confirm_overlay, 0, 0);
    lv_obj_set_style_bg_color(confirm_overlay, lv_color_hex(0x02060B), 0);
    lv_obj_set_style_bg_opa(confirm_overlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(confirm_overlay, 0, 0);
    lv_obj_set_style_pad_all(confirm_overlay, 0, 0);

    lv_obj_t *dialog = lv_obj_create(confirm_overlay);
    lv_obj_set_pos(dialog, 105, 105);
    lv_obj_set_size(dialog, 590, 270);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(dialog, 12, 0);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_pad_all(dialog, 0, 0);

    make_label(dialog, "Messreihe abbrechen?", 28, 25,
               &lv_font_montserrat_24, 0xF4FAFF);
    lv_obj_t *description = make_label(dialog,
        "Möchtest du die laufende Messreihe wirklich abbrechen\nund zur Startseite zurückkehren?",
        28, 82, &lv_font_montserrat_14, 0xAFC2DC);
    lv_obj_set_style_text_line_space(description, 7, 0);
    make_divider(dialog, 28, 148, 534);
    lv_obj_t *continue_button = make_button(
        dialog, "WEITERMACHEN", 28, 178, 250, 64, 0x26384B,
        home_dialog_event, (void *)(uintptr_t)false);
    make_button(dialog, "ZUR STARTSEITE", 312, 178, 250, 64, 0x1455B8,
                home_dialog_event, (void *)(uintptr_t)true);

    if (confirm_group) {
        lv_group_set_default(confirm_background_group);
        encoder_input_activate_group(confirm_group);
        lv_group_focus_obj(continue_button);
    }
}

static void settings_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_settings_async, "settings screen");
}

static void settings_home_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_start_async, "start screen");
}

static void diagnostics_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_diagnostics_async, "diagnostics screen");
}

static void diagnostics_back_event(lv_event_t *event)
{
    (void)event;
    log_operation_error("Stopping two-channel test", gpio_sync_test_stop());
    sync_test_running = false;
    queue_ui_action(build_settings_async, "settings screen");
}

static void sync_test_open_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_sync_test_async, "two-channel test");
}

static void sync_test_back_event(lv_event_t *event)
{
    (void)event;
    log_operation_error("Stopping two-channel test", gpio_sync_test_stop());
    sync_test_running = false;
    queue_ui_action(build_diagnostics_async, "diagnostics screen");
}

static void sync_test_toggle_event(lv_event_t *event)
{
    (void)event;
    const bool starting = !sync_test_running;
    esp_err_t err;
    if (starting) {
        stop_pattern();
        err = gpio_sync_test_start();
        if (err == ESP_OK) sync_test_running = true;
    } else {
        err = gpio_sync_test_stop();
        if (err == ESP_OK) sync_test_running = false;
    }
    log_operation_error(starting ? "Starting two-channel test" :
                                   "Stopping two-channel test", err);
    if (sync_test_status_label) {
        lv_label_set_text(sync_test_status_label,
                          err != ESP_OK ? "FEHLER - DETAILS IM SERIELLEN LOG" :
                          sync_test_running ? "AKTIV - BEIDE KANÄLE MESSEN" : "GESTOPPT");
        lv_obj_set_style_text_color(sync_test_status_label,
                                    lv_color_hex(err != ESP_OK ? 0xE06B6B :
                                                 sync_test_running ? 0x5DE08B : 0x8FA5C2), 0);
    }
    if (sync_test_action_label) {
        lv_label_set_text(sync_test_action_label,
                          sync_test_running ? "TEST STOPPEN" : "TEST STARTEN");
    }
}

static void timer_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_show_timer = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    save_settings();
}

static void values_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_reveal_values = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    save_settings();
}

static void scope_reset_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_scope_reset = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    save_settings();
}

static void encoder_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_encoder_enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    encoder_input_set_enabled(setting_encoder_enabled);
    save_settings();
}

static lv_obj_t *make_setting_row(const char *title, const char *description, int y,
                                  bool enabled, lv_event_cb_t callback)
{
    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, 25, y);
    lv_obj_set_size(card, 750, 66);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, title, 22, 8, &lv_font_montserrat_14, 0xDCE8F7);
    make_label(card, description, 22, 34, &lv_font_montserrat_14, 0x8FA5C2);

    lv_obj_t *toggle = lv_switch_create(card);
    lv_obj_set_pos(toggle, 664, 16);
    lv_obj_set_size(toggle, 60, 34);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0x26384B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0x1455B8),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0xF4FAFF), LV_PART_KNOB);
    if (enabled) lv_obj_add_state(toggle, LV_STATE_CHECKED);
    lv_obj_add_event_cb(toggle, callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(toggle, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    return toggle;
}

static void stop_diagnostics_updates(void)
{
    if (diagnostics_timer) {
        lv_timer_del(diagnostics_timer);
        diagnostics_timer = NULL;
    }
    diagnostics_values_label = NULL;
}

static void diagnostics_update(lv_timer_t *timer)
{
    (void)timer;
    if (!diagnostics_values_label) return;

    uint64_t uptime_seconds = (uint64_t)esp_timer_get_time() / 1000000ULL;
    size_t free_heap = esp_get_free_heap_size();
    size_t minimum_heap = esp_get_minimum_free_heap_size();
    size_t internal_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t psram_heap = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    UBaseType_t stack_reserve = uxTaskGetStackHighWaterMark(NULL);
    lv_mem_monitor_t lv_memory;
    lv_mem_monitor(&lv_memory);

    lv_label_set_text_fmt(
        diagnostics_values_label,
        "%s (%d)\n"
        "%02llu:%02llu:%02llu\n"
        "%lu KB\n"
        "%lu KB\n"
        "%lu KB\n"
        "%lu KB\n"
        "%lu KB\n"
        "%lu KB\n"
        "%u %%\n"
        "%lu Bytes",
        reset_reason_name(esp_reset_reason()), (int)esp_reset_reason(),
        (unsigned long long)(uptime_seconds / 3600ULL),
        (unsigned long long)((uptime_seconds / 60ULL) % 60ULL),
        (unsigned long long)(uptime_seconds % 60ULL),
        (unsigned long)(free_heap / 1024U),
        (unsigned long)(minimum_heap / 1024U), (unsigned long)(internal_heap / 1024U),
        (unsigned long)(psram_heap / 1024U),
        (unsigned long)(largest_block / 1024U),
        (unsigned long)(lv_memory.free_size / 1024U), lv_memory.frag_pct,
        (unsigned long)stack_reserve);
}

static void build_start_screen(void)
{
    stop_diagnostics_updates();
    if (splash_timer) {
        lv_timer_del(splash_timer);
        splash_timer = NULL;
    }
    splash_active = false;
    stop_pattern();
    lesson_selected = false;
    log_ui_memory("before start cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;
    page_default_focus = NULL;
    lv_obj_t *brand_title = make_label(ui_Screen1, "ScopeBuddy", 67, 29,
                                       &lv_font_montserrat_24, 0xF4FAFF);

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    (void)brand_title;
    make_divider(ui_Screen1, 25, 92, 750);

    size_t page_count = lesson_page_count();
    if ((size_t)lesson_page >= page_count) lesson_page = 0;
    size_t first_lesson = (size_t)lesson_page * LESSONS_PER_PAGE;
    for (size_t card = 0; card < LESSONS_PER_PAGE; ++card) {
        const scope_lesson_definition_t *lesson = scopebuddy_lesson_at(first_lesson + card);
        if (lesson) make_mode_card(25 + (int)card * 260, lesson);
    }
    lv_obj_t *output_hint = make_label(
        ui_Screen1,
        "CH1: GPIO48 | CH2: GPIO47 bei Zweikanal-Aufgaben | gemeinsame Masse",
        25, 438, &lv_font_montserrat_14, 0x2684FF);
    lv_obj_set_width(output_hint, 535);
    char page_text[16];
    snprintf(page_text, sizeof(page_text), "%u / %u", lesson_page + 1U,
             (unsigned)page_count);
    lv_obj_t *page_indicator = make_label(ui_Screen1, page_text, 635, 438,
                                          &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_width(page_indicator, 80);
    lv_obj_set_style_text_align(page_indicator, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *previous = make_button(ui_Screen1, LV_SYMBOL_LEFT, 575, 420, 48, 38,
                                     0x26384B, page_event, (void *)(intptr_t)-1);
    lv_obj_t *next = make_button(ui_Screen1, LV_SYMBOL_RIGHT, 727, 420, 48, 38,
                                 0x26384B, page_event, (void *)(intptr_t)1);
    lv_obj_set_style_text_font(lv_obj_get_child(previous, 0), &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(next, 0), &lv_font_montserrat_24, 0);
    if (lesson_page == 0) lv_obj_add_state(previous, LV_STATE_DISABLED);
    if ((size_t)lesson_page + 1U >= page_count) lv_obj_add_state(next, LV_STATE_DISABLED);

    lv_obj_t *settings_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(settings_button, 727, 20);
    lv_obj_set_size(settings_button, 48, 42);
    lv_obj_set_style_radius(settings_button, 8, 0);
    lv_obj_set_style_bg_color(settings_button, lv_color_hex(0x14263A), 0);
    lv_obj_set_style_border_color(settings_button, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(settings_button, 1, 0);
    lv_obj_add_event_cb(settings_button, settings_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(settings_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *settings_symbol = lv_label_create(settings_button);
    lv_label_set_text(settings_symbol, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settings_symbol, &lv_font_montserrat_24, 0);
    lv_obj_center(settings_symbol);
    log_ui_memory("start screen ready");
}

static void build_settings_screen(void)
{
    stop_diagnostics_updates();
    stop_pattern();
    lesson_selected = false;
    log_ui_memory("before settings cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;
    page_default_focus = NULL;

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    make_label(ui_Screen1, "ScopeBuddy", 67, 29, &lv_font_montserrat_24, 0xF4FAFF);
    make_label(ui_Screen1, "EINSTELLUNGEN", 250, 32,
               &lv_font_montserrat_14, 0x2684FF);

    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 727, 20);
    lv_obj_set_size(home_button, 48, 42);
    lv_obj_set_style_radius(home_button, 8, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x14263A), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_remove_style(home_button, NULL, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(home_button, settings_home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(home_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_24, 0);
    lv_obj_center(home_symbol);

    make_divider(ui_Screen1, 25, 92, 750);

    make_setting_row("TIMER EINBLENDEN",
                     "Zeigt die verstrichene Zeit auf den Messaufgabenseiten.",
                     101, setting_show_timer, timer_setting_event);
    make_setting_row("WERTE DIREKT ANZEIGEN",
                     "Zeigt alle Lösungswerte sofort und entfernt die Lösungsbuttons.",
                     173, setting_reveal_values, values_setting_event);
    make_setting_row("AUTO-VORBEREITUNG",
                     "Gibt vor jeder Messaufgabe ein Referenzsignal für AUTO aus.",
                     245, setting_scope_reset, scope_reset_setting_event);
    make_setting_row("ENCODER AKTIVIEREN",
                     "Aktiviert Drehsteuerung und weiße Auswahlkontur.",
                     317, setting_encoder_enabled, encoder_setting_event);

    lv_obj_t *diagnostics_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(diagnostics_button, 25, 397);
    lv_obj_set_size(diagnostics_button, 750, 50);
    lv_obj_set_style_bg_color(diagnostics_button, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_radius(diagnostics_button, 10, 0);
    lv_obj_set_style_border_color(diagnostics_button, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(diagnostics_button, 1, 0);
    lv_obj_add_event_cb(diagnostics_button, diagnostics_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(diagnostics_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    make_label(diagnostics_button, "DIAGNOSE", 8, 8,
               &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_t *diagnostics_arrow = lv_label_create(diagnostics_button);
    lv_label_set_text(diagnostics_arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_pos(diagnostics_arrow, 692, 8);
    lv_obj_set_style_text_font(diagnostics_arrow, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(diagnostics_arrow, lv_color_hex(0x2684FF), 0);
    log_ui_memory("settings ready");
}

static void build_diagnostics_screen(void)
{
    stop_pattern();
    sync_test_status_label = NULL;
    sync_test_action_label = NULL;
    lesson_selected = false;
    stop_diagnostics_updates();
    log_ui_memory("before diagnostics cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;
    page_default_focus = NULL;

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    make_label(ui_Screen1, "ScopeBuddy", 67, 29, &lv_font_montserrat_24, 0xF4FAFF);
    make_label(ui_Screen1, "DIAGNOSE", 250, 32,
               &lv_font_montserrat_14, 0x2684FF);

    lv_obj_t *back_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(back_button, 727, 20);
    lv_obj_set_size(back_button, 48, 42);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x14263A), 0);
    lv_obj_set_style_radius(back_button, 8, 0);
    lv_obj_set_style_border_color(back_button, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(back_button, 1, 0);
    lv_obj_remove_style(back_button, NULL, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(back_button, diagnostics_back_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(back_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *settings_symbol = lv_label_create(back_button);
    lv_label_set_text(settings_symbol, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settings_symbol, &lv_font_montserrat_24, 0);
    lv_obj_center(settings_symbol);
    lv_obj_t *sync_test_button = make_button(ui_Screen1, "2-KANAL-TEST", 570, 20, 140, 42,
                                             0x1455B8, sync_test_open_event, NULL);
    lv_obj_set_style_radius(sync_test_button, 8, 0);
    make_divider(ui_Screen1, 25, 92, 750);

    lv_obj_t *software_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(software_card, 25, 108);
    lv_obj_set_size(software_card, 350, 334);
    lv_obj_clear_flag(software_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(software_card, 10, 0);
    lv_obj_set_style_bg_color(software_card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(software_card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(software_card, 1, 0);
    lv_obj_set_style_pad_all(software_card, 0, 0);
    make_label(software_card, "SOFTWARE & HARDWARE", 20, 16,
               &lv_font_montserrat_14, 0x2684FF);
    make_divider(software_card, 20, 46, 310);

    const esp_app_desc_t *app = esp_app_get_description();
    lv_obj_t *software_keys = make_label(
        software_card,
        "Firmware:\nBuild:\nBuild-ID:\nESP-IDF:\nTarget:\nMesssignal:\nEncoder CLK:\nEncoder DT:\nEncoder Taster:",
        20, 62, &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_style_text_line_space(software_keys, 10, 0);
    char software_text[256];
    snprintf(software_text, sizeof(software_text),
             "%s\n%s %s\n%s\n%s\n%s\nGPIO48 (%s)\nGPIO%d\nGPIO%d\nGPIO%d",
             SCOPEBUDDY_FIRMWARE_VERSION, app->date, app->time, app->version,
             app->idf_ver, CONFIG_IDF_TARGET, hardware_ready ? "bereit" : "nicht bereit",
             ENCODER_GPIO_CLK, ENCODER_GPIO_DT, ENCODER_GPIO_SW);
    lv_obj_t *software_values = make_label(software_card, software_text, 145, 62,
                                            &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_set_style_text_line_space(software_values, 10, 0);

    lv_obj_t *status_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(status_card, 390, 108);
    lv_obj_set_size(status_card, 385, 334);
    lv_obj_clear_flag(status_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(status_card, 10, 0);
    lv_obj_set_style_bg_color(status_card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(status_card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(status_card, 1, 0);
    lv_obj_set_style_pad_all(status_card, 0, 0);
    make_label(status_card, "LAUFZEIT & SPEICHER", 20, 16,
               &lv_font_montserrat_14, 0x2684FF);
    make_divider(status_card, 20, 46, 345);
    lv_obj_t *status_keys = make_label(
        status_card,
        "Resetgrund:\nLaufzeit:\nHeap frei:\nHeap-Minimum:\nIntern frei:\nPSRAM frei:\nGrößter Block:\nLVGL frei:\nLVGL fragmentiert:\nUI-Stackreserve:",
        20, 62, &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_style_text_line_space(status_keys, 10, 0);
    diagnostics_values_label = make_label(status_card, "", 165, 62,
                                           &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_set_style_text_line_space(diagnostics_values_label, 10, 0);
    diagnostics_update(NULL);
    diagnostics_timer = lv_timer_create(diagnostics_update, 1000, NULL);
    if (!diagnostics_timer) ESP_LOGE(UI_TAG, "Creating diagnostics timer failed");

    lv_group_t *encoder_group = lv_group_get_default();
    if (encoder_group) lv_group_focus_obj(back_button);
    log_ui_memory("diagnostics ready");
}

static void build_sync_test_screen(void)
{
    stop_diagnostics_updates();
    stop_pattern();
    sync_test_running = false;
    lesson_selected = false;
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;
    page_default_focus = NULL;

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    make_label(ui_Screen1, "ScopeBuddy", 67, 29, &lv_font_montserrat_24, 0xF4FAFF);
    make_label(ui_Screen1, "2-KANAL-HARDWARETEST", 250, 32,
               &lv_font_montserrat_14, 0x2684FF);

    lv_obj_t *back_button = make_button(ui_Screen1, LV_SYMBOL_LEFT, 727, 20, 48, 42,
                                        0x14263A, sync_test_back_event, NULL);
    lv_obj_set_style_radius(back_button, 8, 0);
    make_divider(ui_Screen1, 25, 92, 750);

    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, 80, 118);
    lv_obj_set_size(card, 640, 320);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, "SYNCHRONISIERTE RMT-AUSGÄNGE", 28, 24,
               &lv_font_montserrat_24, 0xF4FAFF);
    lv_obj_t *description = make_label(
        card,
        "CH1 an GPIO48: 1 kHz, 50 % Tastgrad\n"
        "CH2 an GPIO47: 1 kHz, 50 % Tastgrad\n"
        "Erwarteter Versatz CH1 -> CH2: 100 µs\n"
        "Beide Tastköpfe gegen denselben GND anschließen.",
        28, 74, &lv_font_montserrat_14, 0xAFC2DC);
    lv_obj_set_style_text_line_space(description, 8, 0);

    sync_test_status_label = make_label(card, "GESTOPPT", 28, 205,
                                        &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_t *action_button = make_button(card, "TEST STARTEN", 360, 224, 250, 62,
                                          0x1455B8, sync_test_toggle_event, NULL);
    sync_test_action_label = lv_obj_get_child(action_button, 0);
    page_default_focus = action_button;

    lv_group_t *encoder_group = lv_group_get_default();
    if (encoder_group) lv_group_focus_obj(action_button);
    log_ui_memory("two-channel test ready");
}

static void build_scope_reset_screen(void)
{
    log_ui_memory("before reset-page cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;
    page_default_focus = NULL;

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    make_label(ui_Screen1, "ScopeBuddy", 67, 29, &lv_font_montserrat_24, 0xF4FAFF);

    char heading[80];
    snprintf(heading, sizeof(heading), "STUFE %u / 3", question_number);
    make_label(ui_Screen1, heading, 250, 32, &lv_font_montserrat_14, 0x2684FF);
    make_label(ui_Screen1, mode_name(), 535, 32,
               &lv_font_montserrat_14, 0x8FA5C2);

    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 727, 20);
    lv_obj_set_size(home_button, 48, 42);
    lv_obj_set_style_radius(home_button, 8, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x14263A), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(home_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_24, 0);
    lv_obj_center(home_symbol);

    make_divider(ui_Screen1, 25, 96, 750);

    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, 80, 130);
    lv_obj_set_size(card, 640, 270);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, "AUTO-VORBEREITUNG", 32, 28,
               &lv_font_montserrat_24, 0xF4FAFF);
    const char *auto_instruction = challenge.lesson && challenge.lesson->required_channels == 2 ?
        "Aktiviere CH1 und CH2 mit gleicher Skalierung, triggere auf CH1\n"
        "und drücke dann AUTO am Oszilloskop." :
        "Drücke jetzt den AUTO-Button am Oszilloskop,\n"
        "um die Ansicht für die Messung vorzubereiten.";
    lv_obj_t *instruction = make_label(
        card, auto_instruction,
        32, 88, &lv_font_montserrat_14, 0xAFC2DC);
    lv_obj_set_style_text_line_space(instruction, 8, 0);

    lv_obj_t *button = make_button(card, "ZUR MESSAUFGABE", 350, 182, 258, 58,
                                   0x1455B8, begin_question_event, NULL);
    lv_obj_set_style_radius(button, 9, 0);
    page_default_focus = button;

    lv_group_t *encoder_group = lv_group_get_default();
    if (encoder_group) lv_group_focus_obj(button);

    log_ui_memory("reset page ready");
    if (hardware_ready) start_scope_reset_signal();
}

static void build_question_screen(void)
{
    log_ui_memory("before question cleanup");
    lv_obj_clean(ui_Screen1);
    timer_label = NULL;
    page_default_focus = NULL;
    action_button = NULL;
    action_label = NULL;
    all_values_button = NULL;
    all_values_label = NULL;
    advance_button = NULL;
    for (uint8_t i = 0; i < SCOPEBUDDY_MAX_MEASUREMENTS; ++i) {
        measurement_values[i] = NULL;
        measurement_boxes[i] = NULL;
        measurement_marks[i] = NULL;
        measurement_selected[i] = false;
        measurement_revealed[i] = false;
    }
    elapsed_seconds = 0;
    char heading[80];
    snprintf(heading, sizeof(heading), "STUFE %u / 3", question_number);
    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 727, 20);
    lv_obj_set_size(home_button, 48, 42);
    lv_obj_set_style_radius(home_button, 8, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x14263A), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x2684FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(home_button, encoder_input_touch_event, LV_EVENT_PRESSED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_24, 0);
    lv_obj_center(home_symbol);

    lv_obj_t *brand_logo = lv_img_create(ui_Screen1);
    lv_img_set_src(brand_logo, &ui_img_scopebuddy_logo);
    lv_obj_set_pos(brand_logo, 25, 18);
    make_label(ui_Screen1, "ScopeBuddy", 67, 29, &lv_font_montserrat_24, 0xF4FAFF);
    make_label(ui_Screen1, heading, 250, 32, &lv_font_montserrat_14, 0x2684FF);
    make_label(ui_Screen1, mode_name(), setting_show_timer ? 470 : 570, 32,
               &lv_font_montserrat_14, 0x8FA5C2);
    if (setting_show_timer) {
        timer_label = make_label(ui_Screen1, "00:00", 630, 26,
                                 &lv_font_montserrat_24, 0xDCE8F7);
    }
    make_divider(ui_Screen1, 25, 96, 750);

    lv_obj_t *context = make_label(ui_Screen1, challenge.context, 28, 103,
                                   &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_width(context, 740);
    lv_obj_t *connection = make_label(ui_Screen1, challenge.lesson->connection_hint, 28, 128,
                                      &lv_font_montserrat_14, 0x18B8C9);
    lv_obj_set_width(connection, 740);
    lv_obj_t *trigger = make_label(ui_Screen1, challenge.lesson->trigger_hint, 28, 151,
                                   &lv_font_montserrat_14, 0xE6B43C);
    lv_obj_set_width(trigger, 740);
    make_label(ui_Screen1, "BESTIMME FOLGENDE WERTE:", 28, 176,
               &lv_font_montserrat_14, 0xDCE8F7);
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        make_measurement_item(ui_Screen1, &challenge.measurements[i], 207 + i * 42, i);
    }

    make_divider(ui_Screen1, 25, 346, 750);
    if (!setting_reveal_values) {
        all_values_button = make_button(ui_Screen1, "ALLE WERTE\nANZEIGEN", 25, 386, 235, 56,
                                        0x1455B8, solve_all_event, NULL);
        lv_obj_set_style_radius(all_values_button, 9, 0);
        lv_obj_set_style_bg_opa(all_values_button, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(all_values_button, lv_color_hex(0x2684FF), 0);
        lv_obj_set_style_border_width(all_values_button, 2, 0);
        lv_obj_set_style_shadow_width(all_values_button, 0, 0);
        all_values_label = lv_obj_get_child(all_values_button, 0);
        lv_obj_set_style_text_color(all_values_label, lv_color_hex(0xDCE8F7), 0);
        action_button = make_button(ui_Screen1, "AUSGEWÄHLTE WERTE\nANZEIGEN", 282, 386, 235, 56,
                                    0x1455B8, solve_event, NULL);
        lv_obj_set_style_radius(action_button, 9, 0);
        lv_obj_set_style_bg_opa(action_button, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(action_button, lv_color_hex(0x2684FF), 0);
        lv_obj_set_style_border_width(action_button, 2, 0);
        lv_obj_set_style_shadow_width(action_button, 0, 0);
        action_label = lv_obj_get_child(action_button, 0);
        lv_obj_set_style_text_color(action_label, lv_color_hex(0xDCE8F7), 0);
        lv_obj_add_state(action_button, LV_STATE_DISABLED);
        lv_obj_set_style_border_color(action_button, lv_color_hex(0x34465A), LV_STATE_DISABLED);
        lv_obj_set_style_text_color(action_label, lv_color_hex(0x607895), LV_STATE_DISABLED);
        update_solution_buttons();
    }
    advance_button = make_button(ui_Screen1,
                                 question_number >= 3 ? "ZUR MESSAUFGABEN-\nAUSWAHL" :
                                                        "NÄCHSTE\nMESSAUFGABE",
                                 539, 386, 236, 56, 0x1455B8, advance_event, NULL);
    lv_obj_set_style_radius(advance_button, 9, 0);

    lv_group_t *encoder_group = lv_group_get_default();
    if (encoder_group) {
        page_default_focus = all_values_button ? all_values_button : advance_button;
        lv_group_focus_obj(page_default_focus);
    }

    log_ui_memory("question ready");
    if (hardware_ready) start_challenge_signal();
    else if (action_button) lv_obj_add_state(action_button, LV_STATE_DISABLED);
    apply_direct_value_setting();
}

void GameUiBuildStart(void)
{
    load_settings();
    ensure_pattern_timer();
    if (!game_clock) {
        game_clock = lv_timer_create(clock_callback, 1000, NULL);
        if (!game_clock) ESP_LOGE(UI_TAG, "Creating game clock failed");
    }
    build_splash_screen();
}

void WaveHardwareReady(bool ready)
{
    hardware_ready = ready;
    if (!lesson_selected && !splash_active) build_start_screen();
}

/* Compatibility hooks retained for the generated SquareLine wrapper. */
void LedOn(lv_event_t *e)
{
    (void)e;
    log_operation_error("Starting GPIO48 output", gpio_wave_start());
}
void LedOff(lv_event_t *e)
{
    (void)e;
    log_operation_error("Stopping GPIO48 output", gpio_wave_stop());
}
void WaveFrequencyChanged(lv_event_t *e) { (void)e; }
void WaveDutyChanged(lv_event_t *e) { (void)e; }
void WaveRandomChallenge(lv_event_t *e) { (void)e; }
void WaveRevealChallenge(lv_event_t *e) { (void)e; }
void WaveUiInit(void) { }
