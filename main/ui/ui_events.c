// Aiuto-Vista UI event handlers and screens.
// Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy).
// Layout targets a 320x240 landscape display (Cheap Yellow Display).
#include "ui.h"
#include "bsp_display.h"
#include "bsp_extra.h"
#include "scopebuddy_lessons.h"
#include "scopebuddy_output.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <math.h>

#define AIUTO_VISTA_FIRMWARE_VERSION "0.6.0"
#define UI_TAG "AIUTO_VISTA_UI"
#define SETTINGS_NAMESPACE "aiuto_vista"
#define SETTINGS_KEY "ui_flags"
#define SETTING_FLAG_TIMER       (1U << 0)
#define SETTING_FLAG_VALUES      (1U << 1)
#define SETTING_FLAG_SCOPE_RESET (1U << 2)
#define SETTING_FLAG_SKIP_CONFIRM (1U << 4)
#define LESSONS_PER_PAGE         2U
#define UI_CONTENT_TOP           44
#define LESSON_PAGE_TRANSITION_MS 160U
#define LESSON_SWIPE_MIN_DISTANCE 40
#define LESSON_CARD_PITCH        150

LV_FONT_DECLARE(scopebuddy_font_10);
LV_FONT_DECLARE(scopebuddy_font_16);

static bool hardware_ready;
static bool alternating_state_b;
static uint32_t question_number;
static bool lesson_selected;
static scope_lesson_id_t selected_lesson;
static scope_lesson_instance_t challenge;
static uint8_t lesson_page;
static esp_timer_handle_t pattern_timer;
static StaticSemaphore_t pattern_mutex_buffer;
static SemaphoreHandle_t pattern_mutex;
static bool pattern_enabled;
static uint32_t recent_signatures[10];
static uint8_t recent_count;

static size_t lesson_page_count(void)
{
    return (scopebuddy_lesson_count() + LESSONS_PER_PAGE - 1U) / LESSONS_PER_PAGE;
}

static const uint32_t lesson_accent_palette[] = {
    0xFF0000U, /* rot */
    0xFFFF00U, /* gelb */
    0x00FF00U, /* grün */
    0x00FFFFU, /* cyan */
    0x00A0FFU, /* blau */
    0xFF00FFU, /* magenta */
};

static uint32_t lesson_accent(size_t lesson_index)
{
    const size_t palette_size =
        sizeof(lesson_accent_palette) / sizeof(lesson_accent_palette[0]);
    return lesson_accent_palette[lesson_index % palette_size];
}

static lv_obj_t *action_button;
static lv_obj_t *action_label;
static lv_obj_t *all_values_button;
static lv_obj_t *all_values_label;
static lv_timer_t *game_clock;
static lv_timer_t *splash_timer;
static lv_timer_t *diagnostics_timer;
static lv_obj_t *timer_label;
static lv_obj_t *diagnostics_values_label;
static lv_obj_t *single_test_status_label;
static lv_obj_t *single_test_action_label;
static bool single_test_running;
static lv_obj_t *sync_test_status_label;
static lv_obj_t *sync_test_action_label;
static bool sync_test_running;
static uint32_t elapsed_seconds;
static lv_obj_t *measurement_values[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *measurement_boxes[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *measurement_marks[SCOPEBUDDY_MAX_MEASUREMENTS];
static lv_obj_t *confirm_overlay;
static bool confirm_action_pending;
static bool measurement_selected[SCOPEBUDDY_MAX_MEASUREMENTS];
static bool measurement_revealed[SCOPEBUDDY_MAX_MEASUREMENTS];
static bool setting_show_timer = true;
static bool setting_reveal_values;
static bool setting_scope_reset = true;
static bool setting_confirm_home = true;
static bool settings_loaded;
static bool splash_active;
static lv_async_cb_t pending_ui_action;
static bool ui_action_scheduled;
static bool advance_action_pending;
static lv_obj_t *lesson_cards_layer;
static uint8_t lesson_page_before_transition;
static int8_t lesson_page_entry_direction;
static bool lesson_page_transition_pending;
static lv_point_t lesson_swipe_start;
static bool lesson_swipe_tracking;
static lv_point_t lesson_card_press_start;
static bool lesson_card_moved;

static void mode_event(lv_event_t *event);
static void page_event(lv_event_t *event);
static void build_splash_screen(void);
static void build_start_screen(void);
static void build_settings_screen(void);
static void build_diagnostics_screen(void);
static void build_hardware_tests_screen(void);
static void build_scope_reset_screen(void);
static void build_question_screen(void);
static void build_calibration_screen(void);
static void reveal_measurement(uint8_t index, const char *value);
static void log_operation_error(const char *operation, esp_err_t err);
static void stop_diagnostics_updates(void);

static uint8_t settings_flags(void)
{
    return (setting_show_timer ? SETTING_FLAG_TIMER : 0U) |
           (setting_reveal_values ? SETTING_FLAG_VALUES : 0U) |
           (setting_scope_reset ? SETTING_FLAG_SCOPE_RESET : 0U) |
           (!setting_confirm_home ? SETTING_FLAG_SKIP_CONFIRM : 0U);
}

static void load_settings(void)
{
    if (settings_loaded) return;
    settings_loaded = true;

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(SETTINGS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(UI_TAG, "No stored settings; using defaults");
        return;
    }
    if (err != ESP_OK) {
        log_operation_error("Opening stored settings", err);
        return;
    }

    uint8_t flags = 0;
    esp_err_t flags_err = nvs_get_u8(handle, SETTINGS_KEY, &flags);
    if (flags_err == ESP_OK) {
        setting_show_timer = (flags & SETTING_FLAG_TIMER) != 0;
        setting_reveal_values = (flags & SETTING_FLAG_VALUES) != 0;
        setting_scope_reset = (flags & SETTING_FLAG_SCOPE_RESET) != 0;
        setting_confirm_home = (flags & SETTING_FLAG_SKIP_CONFIRM) == 0;
        ESP_LOGI(UI_TAG, "Loaded settings flags: 0x%02x", flags);
    } else if (flags_err != ESP_ERR_NVS_NOT_FOUND) {
        log_operation_error("Reading stored settings", flags_err);
    }

    nvs_close(handle);
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
    case ESP_RST_EXT: return "EXT-RESET";
    case ESP_RST_SW: return "SW-RESET";
    case ESP_RST_PANIC: return "PANIC/ASSERT";
    case ESP_RST_INT_WDT: return "INT-WDT";
    case ESP_RST_TASK_WDT: return "TASK-WDT";
    case ESP_RST_WDT: return "WATCHDOG";
    case ESP_RST_DEEPSLEEP: return "DEEP-SLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    case ESP_RST_USB: return "USB";
    case ESP_RST_JTAG: return "JTAG";
    case ESP_RST_EFUSE: return "EFUSE-FEHLER";
    case ESP_RST_PWR_GLITCH: return "PWR-GLITCH";
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
        uint8_t parameter_profile = (uint8_t)(esp_random() % 3U) + 1U;
        err = scopebuddy_generate_lesson(selected_lesson, parameter_profile, &challenge);
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

static void build_hardware_tests_async(void *data)
{
    (void)data;
    build_hardware_tests_screen();
}

static void build_start_async(void *data)
{
    (void)data;
    build_start_screen();
}

static void dispatch_ui_action_async(void *data)
{
    (void)data;
    lv_async_cb_t action = pending_ui_action;
    pending_ui_action = NULL;
    ui_action_scheduled = false;
    advance_action_pending = false;
    if (action) action(NULL);
}

static bool queue_ui_action(lv_async_cb_t callback, const char *name)
{
    pending_ui_action = callback;
    if (ui_action_scheduled) return true;

    ui_action_scheduled = true;
    if (lv_async_call(dispatch_ui_action_async, NULL) != LV_RES_OK) {
        pending_ui_action = NULL;
        ui_action_scheduled = false;
        ESP_LOGE(UI_TAG, "Queuing UI action '%s' failed", name);
        return false;
    }
    return true;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int x, int y,
                            const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label,
                               font == &lv_font_montserrat_16 ? &scopebuddy_font_16 : &scopebuddy_font_10,
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
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
    lv_obj_add_flag(button, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_ext_click_area(button, 2);
    lv_obj_add_event_cb(button, callback, LV_EVENT_RELEASED, user_data);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &scopebuddy_font_10, 0);
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
    lv_obj_set_style_bg_color(line, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
}

static void splash_set_opacity(void *object, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)object, (lv_opa_t)value, 0);
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
    lv_obj_set_size(background, 320, 240);
    lv_obj_clear_flag(background, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(background, 0, 0);
    lv_obj_set_style_bg_color(background, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(background, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(background, 0, 0);
    lv_obj_set_style_pad_all(background, 0, 0);

    lv_obj_t *title = lv_label_create(background);
    lv_label_set_text(title, "Aiuto-Vista");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_opa(title, LV_OPA_TRANSP, 0);

    lv_obj_t *version = make_label(background,
                                   "Firmware-Version " AIUTO_VISTA_FIRMWARE_VERSION,
                                   0, 140, &lv_font_montserrat_10, 0x808080);
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_set_style_opa(version, LV_OPA_TRANSP, 0);

    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, title);
    lv_anim_set_exec_cb(&animation, splash_set_opacity);
    lv_anim_set_values(&animation, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&animation, 420);
    lv_anim_set_delay(&animation, 100);
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
                                lv_color_hex(any_selected ? 0xFFFFFF : 0x808080), 0);
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
    lv_obj_set_style_bg_color(measurement_boxes[index], lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_bg_opa(measurement_boxes[index],
                            measurement_selected[index] ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

    update_solution_buttons();
}

static const char *measurement_source_name(scope_measurement_source_t source)
{
    switch (source) {
    case SCOPE_MEASUREMENT_CHANNEL_1: return "CH1";
    case SCOPE_MEASUREMENT_CHANNEL_2: return "CH2";
    case SCOPE_MEASUREMENT_CHANNEL_PAIR: return "PAAR";
    case SCOPE_MEASUREMENT_DERIVED: return "BER.";
    default: return "";
    }
}

static uint32_t measurement_source_color(scope_measurement_source_t source)
{
    switch (source) {
    case SCOPE_MEASUREMENT_CHANNEL_1: return 0x00A0FF;
    case SCOPE_MEASUREMENT_CHANNEL_2: return 0xFFFF00;
    case SCOPE_MEASUREMENT_CHANNEL_PAIR: return 0x00FFFF;
    case SCOPE_MEASUREMENT_DERIVED: return 0xFF00FF;
    default: return 0x808080;
    }
}

static void make_measurement_item(lv_obj_t *parent, const scope_measurement_t *measurement,
                                  int y, uint8_t index)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, 4, y - 1);
    lv_obj_set_size(box, 12, 12);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(box, 3, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    measurement_boxes[index] = box;
    measurement_marks[index] = make_label(box, "", 3, 1, &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_add_event_cb(box, measurement_event, LV_EVENT_RELEASED,
                        (void *)(uintptr_t)index);
    lv_obj_t *source_label = make_label(
        parent, measurement_source_name(measurement->source), 20, y + 1,
        &lv_font_montserrat_10, measurement_source_color(measurement->source));
    lv_obj_set_width(source_label, 40);
    lv_obj_t *measurement_label = make_label(parent, measurement->label, 50, y + 1,
                                              &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_set_width(measurement_label, 54);
    lv_obj_set_style_text_line_space(measurement_label, 3, 0);
    measurement_values[index] = make_label(parent, "---", 106, y + 1,
                                            &lv_font_montserrat_10, 0x808080);
    lv_obj_set_width(measurement_values[index], 36);
    lv_obj_set_style_text_align(measurement_values[index], LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_t *touch_area = lv_obj_create(parent);
    lv_obj_set_pos(touch_area, 4, y - 3);
    lv_obj_set_size(touch_area, 135, 17);
    lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(touch_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(touch_area, 2);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(touch_area, 0, 0);
    lv_obj_set_style_pad_all(touch_area, 0, 0);
    lv_obj_add_event_cb(touch_area, measurement_event, LV_EVENT_RELEASED,
                        (void *)(uintptr_t)index);
}

static void reveal_measurement(uint8_t index, const char *value)
{
    lv_label_set_text(measurement_values[index], value);
    lv_obj_set_style_text_color(measurement_values[index], lv_color_hex(0x00FF00), 0);
    measurement_revealed[index] = true;
}

static void hide_measurement(uint8_t index)
{
    lv_label_set_text(measurement_values[index], "---");
    lv_obj_set_style_text_color(measurement_values[index], lv_color_hex(0x808080), 0);
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

static void lesson_card_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_indev_t *input = lv_indev_get_act();
    if (!input || lv_indev_get_type(input) != LV_INDEV_TYPE_POINTER) return;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(input, &lesson_card_press_start);
        lesson_card_moved = false;
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (lesson_page_transition_pending) return;
        lv_point_t current;
        lv_indev_get_point(input, &current);
        int32_t dx = current.x - lesson_card_press_start.x;
        int32_t dy = current.y - lesson_card_press_start.y;
        if ((dx < 0 ? -dx : dx) > LESSON_SWIPE_MIN_DISTANCE ||
            (dy < 0 ? -dy : dy) > LESSON_SWIPE_MIN_DISTANCE) {
            lesson_card_moved = true;
        }
        return;
    }

    if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) &&
        !lesson_card_moved && !lesson_page_transition_pending) {
        mode_event(event);
    }
}

static void make_mode_card(lv_obj_t *parent, int x, size_t lesson_index,
                           const scope_lesson_definition_t *lesson)
{
    uint32_t accent = lesson_accent(lesson_index);
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, UI_CONTENT_TOP);
    lv_obj_set_size(card, 140, 156);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_ext_click_area(card, 2);
    lv_obj_set_style_radius(card, 5, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_add_event_cb(card, lesson_card_event, LV_EVENT_ALL, (void *)(uintptr_t)lesson->id);

    lv_obj_set_style_border_color(card, lv_color_hex(accent), 0);
    lv_obj_t *title = make_label(card, lesson->title, 7, 8,
                                 &lv_font_montserrat_16, accent);
    lv_obj_set_width(title, 126);
    lv_obj_set_style_text_font(title, &scopebuddy_font_10, 0);
    make_label(card, lesson->category, 7, 23, &lv_font_montserrat_10, 0xDCE8F7);
    lv_obj_t *description = make_label(card, lesson->summary, 7, 39,
                                       &lv_font_montserrat_10, 0x8FA5C2);
    lv_obj_set_width(description, 126);
    lv_obj_set_style_text_line_space(description, 2, 0);
    char channel_badge[8];
    snprintf(channel_badge, sizeof(channel_badge), "%u CH", lesson->required_channels);
    lv_obj_t *badge = make_label(card, channel_badge, 112, 104,
                                 &lv_font_montserrat_10,
                                 lesson->required_channels == 2 ? 0x18B8C9 : 0x607895);
    lv_obj_set_width(badge, 21);
    lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_RIGHT, 0);
    make_label(card, "TIPPEN ZUM STARTEN", 7, 130,
               &lv_font_montserrat_10, accent);
}

static void mode_event(lv_event_t *event)
{
    if (lesson_page_transition_pending) return;
    selected_lesson = (scope_lesson_id_t)(uintptr_t)lv_event_get_user_data(event);
    const scope_lesson_definition_t *lesson = scopebuddy_lesson_at(selected_lesson);
    if (lesson == NULL) return;
    lesson_selected = true;
    question_number = 1;
    queue_ui_action(prepare_question_async, "prepare question");
}

static void lesson_layer_set_x(void *object, int32_t x)
{
    lv_obj_t *layer = object;
    if (layer && lv_obj_is_valid(layer)) lv_obj_set_x(layer, x);
}

static void lesson_page_entry_ready(lv_anim_t *animation)
{
    (void)animation;
    lesson_page_transition_pending = false;
}

static void lesson_page_exit_ready(lv_anim_t *animation)
{
    (void)animation;
    if (!lesson_page_transition_pending) return;
    if (!queue_ui_action(build_start_async, "lesson page transition")) {
        lesson_page = lesson_page_before_transition;
        lesson_page_entry_direction = 0;
        lesson_page_transition_pending = false;
        if (lesson_cards_layer && lv_obj_is_valid(lesson_cards_layer)) {
            lv_obj_set_x(lesson_cards_layer, 0);
        }
    }
}

static void animate_lesson_layer(lv_obj_t *layer, int32_t start_x, int32_t end_x,
                                 lv_anim_ready_cb_t ready_callback)
{
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, layer);
    lv_anim_set_exec_cb(&animation, lesson_layer_set_x);
    lv_anim_set_values(&animation, start_x, end_x);
    lv_anim_set_time(&animation, LESSON_PAGE_TRANSITION_MS);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    if (ready_callback) lv_anim_set_ready_cb(&animation, ready_callback);
    lv_anim_start(&animation);
}

static void change_lesson_page(intptr_t direction)
{
    size_t page_count = lesson_page_count();
    if (direction == 0 || page_count < 2U || lesson_page_transition_pending ||
        ui_action_scheduled) {
        return;
    }

    lesson_page_before_transition = lesson_page;
    if (direction < 0) {
        lesson_page = lesson_page > 0 ? lesson_page - 1U : (uint8_t)(page_count - 1U);
    }
    if (direction > 0) {
        lesson_page = ((size_t)lesson_page + 1U < page_count) ? lesson_page + 1U : 0U;
    }
    lesson_page_entry_direction = direction > 0 ? 1 : -1;
    lesson_page_transition_pending = true;

    if (lesson_cards_layer && lv_obj_is_valid(lesson_cards_layer)) {
        animate_lesson_layer(lesson_cards_layer, lv_obj_get_x(lesson_cards_layer),
                             -lesson_page_entry_direction * LESSON_CARD_PITCH,
                             lesson_page_exit_ready);
        return;
    }

    if (!queue_ui_action(build_start_async, "lesson page")) {
        lesson_page = lesson_page_before_transition;
        lesson_page_entry_direction = 0;
        lesson_page_transition_pending = false;
    }
}

static void lesson_page_touch_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_indev_t *input = lv_indev_get_act();
    if (!input || lv_indev_get_type(input) != LV_INDEV_TYPE_POINTER) return;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(input, &lesson_swipe_start);
        lesson_swipe_tracking = true;
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lesson_swipe_tracking = false;
        return;
    }

    if (code != LV_EVENT_PRESSING || !lesson_swipe_tracking ||
        lesson_page_transition_pending) {
        return;
    }

    lv_point_t current;
    lv_indev_get_point(input, &current);
    int32_t delta_x = current.x - lesson_swipe_start.x;
    int32_t delta_y = current.y - lesson_swipe_start.y;
    int32_t distance_x = delta_x < 0 ? -delta_x : delta_x;
    int32_t distance_y = delta_y < 0 ? -delta_y : delta_y;

    if (distance_x >= LESSON_SWIPE_MIN_DISTANCE && distance_x > distance_y + 20) {
        lesson_swipe_tracking = false;
        lv_event_stop_bubbling(event);
        change_lesson_page(delta_x < 0 ? 1 : -1);
    }
}

static void page_event(lv_event_t *event)
{
    change_lesson_page((intptr_t)lv_event_get_user_data(event));
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
    if (advance_action_pending) return;
    advance_action_pending = true;

    uint32_t previous_number = question_number;
    question_number = question_number == UINT32_MAX ? 1U : question_number + 1U;
    bool queued = queue_ui_action(prepare_question_async, "next question");
    if (!queued) question_number = previous_number;
    if (!queued) advance_action_pending = false;
}

static void begin_question_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_question_async, "question screen");
}

static void return_home_async(void *data)
{
    (void)data;
    confirm_action_pending = false;
    confirm_overlay = NULL;
    build_start_screen();
}

static void dismiss_confirm_async(void *data)
{
    (void)data;
    lv_obj_t *overlay = confirm_overlay;
    confirm_overlay = NULL;
    confirm_action_pending = false;
    if (overlay && lv_obj_is_valid(overlay)) lv_obj_del(overlay);
}

static void home_dialog_event(lv_event_t *event)
{
    if (confirm_action_pending) return;
    confirm_action_pending = true;

    bool return_home = (bool)(uintptr_t)lv_event_get_user_data(event);
    bool queued;
    if (return_home) {
        stop_pattern();
        queued = queue_ui_action(return_home_async, "return home");
    } else {
        queued = queue_ui_action(dismiss_confirm_async, "dismiss confirmation");
    }
    if (!queued) confirm_action_pending = false;
}

static void home_event(lv_event_t *event)
{
    (void)event;
    if (confirm_overlay || ui_action_scheduled) return;

    if (!setting_confirm_home) {
        stop_pattern();
        queue_ui_action(return_home_async, "return home");
        return;
    }

    confirm_action_pending = false;
    confirm_overlay = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(confirm_overlay, 0, 0);
    lv_obj_set_size(confirm_overlay, 320, 240);
    lv_obj_clear_flag(confirm_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(confirm_overlay, 0, 0);
    lv_obj_set_style_bg_color(confirm_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(confirm_overlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(confirm_overlay, 0, 0);
    lv_obj_set_style_pad_all(confirm_overlay, 0, 0);

    lv_obj_t *dialog = lv_obj_create(confirm_overlay);
    lv_obj_set_pos(dialog, 42, 42);
    lv_obj_set_size(dialog, 236, 108);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(dialog, 6, 0);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_pad_all(dialog, 0, 0);

    make_label(dialog, "Messreihe abbrechen?", 11, 10,
               &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_t *description = make_label(dialog,
        "Möchtest du die laufende Messreihe\nwirklich abbrechen und zurückkehren?",
        11, 33, &lv_font_montserrat_10, 0x808080);
    lv_obj_set_style_text_line_space(description, 3, 0);
    make_divider(dialog, 11, 59, 214);
    lv_obj_t *continue_button = make_button(
        dialog, "WEITER", 11, 71, 100, 26, 0x808080,
        home_dialog_event, (void *)(uintptr_t)false);
    make_button(dialog, "STARTSEITE", 125, 71, 100, 26, 0x00A0FF,
                home_dialog_event, (void *)(uintptr_t)true);
    (void)continue_button;
}

static void settings_event(lv_event_t *event)
{
    (void)event;
    if (lesson_page_transition_pending) return;
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

static void hardware_tests_open_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_hardware_tests_async, "hardware tests");
}

static void hardware_tests_back_event(lv_event_t *event)
{
    (void)event;
    stop_pattern();
    single_test_running = false;
    sync_test_running = false;
    queue_ui_action(build_diagnostics_async, "diagnostics screen");
}

static void update_hardware_test_control(lv_obj_t *status_label, lv_obj_t *action_label,
                                         bool running, const char *active_text, esp_err_t err)
{
    if (status_label) {
        lv_label_set_text(status_label,
                          err != ESP_OK ? "FEHLER - SERIELLEN LOG PRÜFEN" :
                          running ? active_text : "GESTOPPT");
        lv_obj_set_style_text_color(status_label,
                                    lv_color_hex(err != ESP_OK ? 0xFF0000 :
                                                 running ? 0x00FF00 : 0x808080), 0);
    }
    if (action_label) {
        lv_label_set_text(action_label, running ? "TEST STOPPEN" : "TEST STARTEN");
    }
}

static void single_test_toggle_event(lv_event_t *event)
{
    (void)event;
    const bool starting = !single_test_running;
    esp_err_t err;
    if (starting) {
        stop_pattern();
        sync_test_running = false;
        update_hardware_test_control(sync_test_status_label, sync_test_action_label,
                                     false, "", ESP_OK);
        err = gpio_wave_set_frequency(1000);
        if (err == ESP_OK) err = gpio_wave_set_duty(50);
        if (err == ESP_OK) err = gpio_wave_start();
        if (err == ESP_OK) single_test_running = true;
    } else {
        err = gpio_wave_stop();
        if (err == ESP_OK) single_test_running = false;
    }
    log_operation_error(starting ? "Starting one-channel test" :
                                   "Stopping one-channel test", err);
    update_hardware_test_control(single_test_status_label, single_test_action_label,
                                 single_test_running, "AKTIV - GPIO26 MESSEN", err);
}

static void sync_test_toggle_event(lv_event_t *event)
{
    (void)event;
    const bool starting = !sync_test_running;
    esp_err_t err;
    if (starting) {
        stop_pattern();
        single_test_running = false;
        update_hardware_test_control(single_test_status_label, single_test_action_label,
                                     false, "", ESP_OK);
        err = gpio_sync_test_start();
        if (err == ESP_OK) sync_test_running = true;
    } else {
        err = gpio_sync_test_stop();
        if (err == ESP_OK) sync_test_running = false;
    }
    log_operation_error(starting ? "Starting two-channel test" :
                                   "Stopping two-channel test", err);
    update_hardware_test_control(sync_test_status_label, sync_test_action_label,
                                 sync_test_running, "AKTIV - BEIDE KANÄLE MESSEN", err);
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

static void confirm_home_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_confirm_home = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    save_settings();
}

/* Touch calibration: tap 5 crosses (center + 4 corners) in order. Raw touch
 * coordinates are captured (calibration raw mode) and a 6-parameter affine
 * transform is fitted via least squares, then stored per device in NVS. */

static const lv_point_t cal_targets[5] = {
    {160, 120}, {40, 40}, {280, 40}, {280, 200}, {40, 200},
};
static uint16_t cal_raw_x[5];
static uint16_t cal_raw_y[5];
static uint8_t cal_step = 0;
static lv_obj_t *cal_crosses[5];
static lv_obj_t *cal_step_label = NULL;

static void set_cross_color(uint8_t index, uint32_t color)
{
    if (index >= 5 || cal_crosses[index] == NULL) return;
    uint32_t child_count = lv_obj_get_child_cnt(cal_crosses[index]);
    for (uint32_t i = 0; i < child_count; ++i) {
        lv_obj_set_style_bg_color(lv_obj_get_child(cal_crosses[index], i),
                                  lv_color_hex(color), 0);
    }
}

static void update_cal_crosses(void)
{
    for (uint8_t i = 0; i < 5; ++i) {
        uint32_t color = (i < cal_step) ? 0x00FF00 : (i == cal_step) ? 0x00A0FF : 0xFFFFFF;
        set_cross_color(i, color);
    }
    if (cal_step_label) {
        char step_text[24];
        snprintf(step_text, sizeof(step_text), "TIPPE: KREUZ %u/5",
                 (unsigned)(cal_step < 5 ? cal_step + 1 : 5));
        lv_label_set_text(cal_step_label, step_text);
    }
}

static void make_cross(lv_obj_t *parent, const lv_point_t *pos, uint8_t index)
{
    lv_obj_t *cross = lv_obj_create(parent);
    lv_obj_set_pos(cross, pos->x - 7, pos->y - 7);
    lv_obj_set_size(cross, 14, 14);
    lv_obj_clear_flag(cross, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(cross, 0, 0);
    lv_obj_set_style_bg_opa(cross, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cross, 0, 0);
    lv_obj_set_style_pad_all(cross, 0, 0);
    lv_obj_t *h = lv_obj_create(cross);
    lv_obj_set_pos(h, 0, 5);
    lv_obj_set_size(h, 14, 3);
    lv_obj_clear_flag(h, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(h, 1, 0);
    lv_obj_set_style_border_width(h, 0, 0);
    lv_obj_set_style_pad_all(h, 0, 0);
    lv_obj_set_style_bg_opa(h, LV_OPA_COVER, 0);
    lv_obj_t *v = lv_obj_create(cross);
    lv_obj_set_pos(v, 5, 0);
    lv_obj_set_size(v, 3, 14);
    lv_obj_clear_flag(v, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(v, 1, 0);
    lv_obj_set_style_border_width(v, 0, 0);
    lv_obj_set_style_pad_all(v, 0, 0);
    lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);
    cal_crosses[index] = cross;
}

static bool solve_linear3(float m[3][4], float out[3])
{
    for (int col = 0; col < 3; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 3; ++row) {
            if (fabsf(m[row][col]) > fabsf(m[pivot][col])) pivot = row;
        }
        if (fabsf(m[pivot][col]) < 1e-6f) return false;
        if (pivot != col) {
            for (int c = 0; c < 4; ++c) {
                float tmp = m[col][c];
                m[col][c] = m[pivot][c];
                m[pivot][c] = tmp;
            }
        }
        for (int row = 0; row < 3; ++row) {
            if (row == col) continue;
            float factor = m[row][col] / m[col][col];
            for (int c = col; c < 4; ++c) {
                m[row][c] -= factor * m[col][c];
            }
        }
    }
    for (int i = 0; i < 3; ++i) out[i] = m[i][3] / m[i][i];
    return true;
}

static void compute_calibration(float params[6])
{
    float Sxx = 0.0f, Syy = 0.0f, Sxy = 0.0f, Sx = 0.0f, Sy = 0.0f;
    float Txx = 0.0f, Tyx = 0.0f, Tx = 0.0f;
    float Tyy = 0.0f, Txy = 0.0f, Ty = 0.0f;
    for (int i = 0; i < 5; ++i) {
        float rx = (float)cal_raw_x[i];
        float ry = (float)cal_raw_y[i];
        float tx = (float)cal_targets[i].x;
        float ty = (float)cal_targets[i].y;
        Sxx += rx * rx; Syy += ry * ry; Sxy += rx * ry;
        Sx += rx; Sy += ry;
        Txx += rx * tx; Tyx += ry * tx; Tx += tx;
        Tyy += rx * ty; Txy += ry * ty; Ty += ty;
    }
    float mx[3][4] = {{Sxx, Sxy, Sx, Txx}, {Sxy, Syy, Sy, Tyx}, {Sx, Sy, 5.0f, Tx}};
    float my[3][4] = {{Sxx, Sxy, Sx, Tyy}, {Sxy, Syy, Sy, Txy}, {Sx, Sy, 5.0f, Ty}};
    float ox[3], oy[3];
    if (!solve_linear3(mx, ox) || !solve_linear3(my, oy)) {
        touch_calibration_load(params);
        return;
    }
    params[0] = ox[0]; params[1] = ox[1]; params[2] = ox[2];
    params[3] = oy[0]; params[4] = oy[1]; params[5] = oy[2];
}

static void calibration_tap_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_RELEASED || cal_step >= 5) return;

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);

    cal_raw_x[cal_step] = (uint16_t)point.x;
    cal_raw_y[cal_step] = (uint16_t)point.y;
    cal_step++;
    update_cal_crosses();
    if (cal_step < 5) return;

    float params[6];
    compute_calibration(params);
    esp_err_t err = touch_calibration_save(params);
    touch_set_raw_mode(false);

    if (err == ESP_OK) {
        make_label(ui_Screen1, "KALIBRIERUNG GESPEICHERT", 10, 60,
                   &lv_font_montserrat_16, 0x00FF00);
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "SPEICHERN FEHLGESCHLAGEN: %s", esp_err_to_name(err));
        make_label(ui_Screen1, msg, 10, 60, &lv_font_montserrat_10, 0xFF0000);
    }
    lv_obj_t *done = make_button(ui_Screen1, "FERTIG", 113, 130, 94, 30,
                                 0x00A0FF, settings_event, NULL);
    lv_obj_set_style_radius(done, 5, 0);
    lv_obj_set_style_shadow_width(done, 0, 0);
}

static void build_calibration_async(void *data)
{
    (void)data;
    build_calibration_screen();
}

static void build_calibration_screen(void)
{
    stop_diagnostics_updates();
    stop_pattern();
    lesson_selected = false;
    log_ui_memory("before calibration cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;

    cal_step = 0;
    touch_set_raw_mode(true);

    make_label(ui_Screen1, "TOUCH-KALIBRIERUNG", 10, 12,
               &lv_font_montserrat_16, 0xFFFFFF);
    cal_step_label = make_label(ui_Screen1, "TIPPE: KREUZ 1/5", 200, 13,
                                &lv_font_montserrat_10, 0x00A0FF);

    lv_obj_t *layer = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(layer, 0, 0);
    lv_obj_set_size(layer, 320, 240);
    lv_obj_clear_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(layer, 0, 0);
    lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(layer, 0, 0);
    lv_obj_set_style_pad_all(layer, 0, 0);
    lv_obj_add_event_cb(layer, calibration_tap_event, LV_EVENT_ALL, NULL);

    for (uint8_t i = 0; i < 5; ++i) {
        make_cross(layer, &cal_targets[i], i);
    }
    update_cal_crosses();

    make_label(ui_Screen1, "TIPPE NACHEINANDER AUF JEDES KREUZ.",
               10, 224, &lv_font_montserrat_10, 0x808080);
    log_ui_memory("calibration ready");
}

static void calibration_event(lv_event_t *event)
{
    (void)event;
    queue_ui_action(build_calibration_async, "calibration screen");
}

static lv_obj_t *make_setting_row(lv_obj_t *parent, const char *title,
                                  const char *description, int y, bool enabled,
                                  bool show_divider, lv_event_cb_t callback)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_size(row, 300, 30);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    make_label(row, title, 9, 2, &lv_font_montserrat_10, 0xFFFFFF);
    make_label(row, description, 9, 16, &lv_font_montserrat_10, 0x808080);

    lv_obj_t *toggle = lv_switch_create(row);
    lv_obj_set_pos(toggle, 266, 5);
    lv_obj_set_size(toggle, 24, 14);
    lv_obj_set_ext_click_area(toggle, 2);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0x00A0FF),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(toggle, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    if (enabled) lv_obj_add_state(toggle, LV_STATE_CHECKED);
    lv_obj_add_event_cb(toggle, callback, LV_EVENT_VALUE_CHANGED, NULL);
    if (show_divider) make_divider(parent, 9, y + 31, 282);
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
    size_t minimum_heap = esp_get_minimum_free_heap_size();
    size_t internal_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    lv_mem_monitor_t lv_memory;
    lv_mem_monitor(&lv_memory);
    UBaseType_t stack_reserve = uxTaskGetStackHighWaterMark(NULL);

    lv_label_set_text_fmt(
        diagnostics_values_label,
        "%s (%d)\n"
        "%02llu:%02llu:%02llu\n"
        "%lu KB\n"
        "%lu KB\n"
        "%lu KB\n"
        "%u %%\n"
        "%lu Bytes",
        reset_reason_name(esp_reset_reason()), (int)esp_reset_reason(),
        (unsigned long long)(uptime_seconds / 3600ULL),
        (unsigned long long)((uptime_seconds / 60ULL) % 60ULL),
        (unsigned long long)(uptime_seconds % 60ULL),
        (unsigned long)(minimum_heap / 1024U), (unsigned long)(internal_heap / 1024U),
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
    lesson_cards_layer = NULL;
    confirm_overlay = NULL;
    timer_label = NULL;
    make_label(ui_Screen1, "Aiuto-Vista", 10, 12, &lv_font_montserrat_16, 0xFFFFFF);
    size_t page_count = lesson_page_count();
    if ((size_t)lesson_page >= page_count) lesson_page = 0;

    lesson_cards_layer = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(lesson_cards_layer, 0, 0);
    lv_obj_set_size(lesson_cards_layer, 320, 204);
    lv_obj_clear_flag(lesson_cards_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lesson_cards_layer,
                    LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_radius(lesson_cards_layer, 0, 0);
    lv_obj_set_style_bg_opa(lesson_cards_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lesson_cards_layer, 0, 0);
    lv_obj_set_style_shadow_width(lesson_cards_layer, 0, 0);
    lv_obj_set_style_pad_all(lesson_cards_layer, 0, 0);
    lv_obj_add_event_cb(lesson_cards_layer, lesson_page_touch_event,
                        LV_EVENT_ALL, NULL);

    size_t first_lesson = (size_t)lesson_page * LESSONS_PER_PAGE;
    for (size_t card = 0; card < LESSONS_PER_PAGE; ++card) {
        size_t lesson_index = first_lesson + card;
        const scope_lesson_definition_t *lesson = scopebuddy_lesson_at(lesson_index);
        if (lesson) {
            make_mode_card(lesson_cards_layer, 10 + (int)card * LESSON_CARD_PITCH,
                           lesson_index, lesson);
        }
    }

    int8_t entry_direction = lesson_page_entry_direction;
    lesson_page_entry_direction = 0;
    if (entry_direction != 0) {
        int32_t start_x = entry_direction * LESSON_CARD_PITCH;
        lv_obj_set_x(lesson_cards_layer, start_x);
        animate_lesson_layer(lesson_cards_layer, start_x, 0,
                             lesson_page_entry_ready);
    } else {
        lesson_page_transition_pending = false;
    }
    make_label(ui_Screen1, "CH1", 10, 222,
               &lv_font_montserrat_10, 0x00A0FF);
    make_label(ui_Screen1, "GPIO26", 31, 222,
               &lv_font_montserrat_10, 0xFFFFFF);
    make_label(ui_Screen1, "CH2", 98, 222,
               &lv_font_montserrat_10, 0x00A0FF);
    make_label(ui_Screen1, "GPIO27", 119, 222,
               &lv_font_montserrat_10, 0xFFFFFF);
    make_label(ui_Screen1, "GND", 170, 222,
               &lv_font_montserrat_10, 0x00A0FF);
    make_label(ui_Screen1, "JACK", 194, 222,
               &lv_font_montserrat_10, 0xFFFFFF);
    char page_text[16];
    snprintf(page_text, sizeof(page_text), "%u / %u", lesson_page + 1U,
             (unsigned)page_count);
    lv_obj_t *page_indicator = make_label(ui_Screen1, page_text, 248, 212,
                                          &lv_font_montserrat_10, 0x808080);
    lv_obj_set_width(page_indicator, 40);
    lv_obj_set_style_text_align(page_indicator, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *previous = make_button(ui_Screen1, LV_SYMBOL_LEFT, 226, 206, 20, 26,
                                     0x808080, page_event, (void *)(intptr_t)-1);
    lv_obj_t *next = make_button(ui_Screen1, LV_SYMBOL_RIGHT, 290, 206, 20, 26,
                                 0x808080, page_event, (void *)(intptr_t)1);
    lv_obj_set_style_text_font(lv_obj_get_child(previous, 0), &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(next, 0), &lv_font_montserrat_16, 0);

    lv_obj_t *settings_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(settings_button, 262, 6);
    lv_obj_set_size(settings_button, 50, 26);
    lv_obj_set_style_radius(settings_button, 4, 0);
    lv_obj_set_style_bg_color(settings_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(settings_button, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(settings_button, 1, 0);
    lv_obj_add_event_cb(settings_button, settings_event, LV_EVENT_RELEASED, NULL);
    lv_obj_t *settings_symbol = lv_label_create(settings_button);
    lv_label_set_text(settings_symbol, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settings_symbol, &lv_font_montserrat_16, 0);
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

    make_label(ui_Screen1, "Aiuto-Vista", 10, 12, &lv_font_montserrat_16, 0xFFFFFF);
    make_label(ui_Screen1, "EINSTELLUNGEN", 100, 13,
               &lv_font_montserrat_10, 0x00A0FF);

    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 262, 6);
    lv_obj_set_size(home_button, 50, 26);
    lv_obj_set_style_radius(home_button, 4, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, settings_home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_16, 0);
    lv_obj_center(home_symbol);

    lv_obj_t *settings_panel = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(settings_panel, 10, UI_CONTENT_TOP);
    lv_obj_set_size(settings_panel, 300, 154);
    lv_obj_clear_flag(settings_panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(settings_panel, 5, 0);
    lv_obj_set_style_bg_color(settings_panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(settings_panel, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(settings_panel, 1, 0);
    lv_obj_set_style_pad_all(settings_panel, 0, 0);

    make_setting_row(settings_panel, "WERTE DIREKT ANZEIGEN",
                     "Zeigt Lösungswerte sofort an.",
                     5, setting_reveal_values, true, values_setting_event);
    make_setting_row(settings_panel, "AUTO-VORBEREITUNG",
                     "Referenzsignal vor jeder Aufgabe.",
                     43, setting_scope_reset, true, scope_reset_setting_event);
    make_setting_row(settings_panel, "ABBRUCH BESTÄTIGEN",
                     "Nachfrage vor Rückkehr zur Startseite.",
                     81, setting_confirm_home, false, confirm_home_setting_event);

    lv_obj_t *calibration_button = lv_btn_create(settings_panel);
    lv_obj_set_pos(calibration_button, 0, 119);
    lv_obj_set_size(calibration_button, 300, 30);
    lv_obj_set_style_bg_color(calibration_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(calibration_button, 0, 0);
    lv_obj_set_style_border_width(calibration_button, 0, 0);
    lv_obj_set_style_shadow_width(calibration_button, 0, 0);
    lv_obj_set_style_pad_all(calibration_button, 0, 0);
    lv_obj_add_event_cb(calibration_button, calibration_event, LV_EVENT_RELEASED, NULL);
    make_label(calibration_button, "TOUCH-KALIBRIERUNG", 9, 2,
               &lv_font_montserrat_10, 0xFFFFFF);
    make_label(calibration_button, "Display auf Berührung abgleichen.", 9, 16,
               &lv_font_montserrat_10, 0x808080);
    lv_obj_t *cal_arrow = lv_label_create(calibration_button);
    lv_label_set_text(cal_arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_pos(cal_arrow, 268, 3);
    lv_obj_set_width(cal_arrow, 24);
    lv_obj_set_style_text_font(cal_arrow, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(cal_arrow, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_text_align(cal_arrow, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *diagnostics_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(diagnostics_button, 10, 206);
    lv_obj_set_size(diagnostics_button, 300, 22);
    lv_obj_set_style_bg_color(diagnostics_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(diagnostics_button, 5, 0);
    lv_obj_set_style_border_color(diagnostics_button, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(diagnostics_button, 1, 0);
    lv_obj_set_style_shadow_width(diagnostics_button, 0, 0);
    lv_obj_set_style_pad_all(diagnostics_button, 0, 0);
    lv_obj_add_event_cb(diagnostics_button, diagnostics_event, LV_EVENT_RELEASED, NULL);
    make_label(diagnostics_button, "DIAGNOSE", 9, 4,
               &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_t *diagnostics_arrow = lv_label_create(diagnostics_button);
    lv_label_set_text(diagnostics_arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_pos(diagnostics_arrow, 266, 1);
    lv_obj_set_width(diagnostics_arrow, 24);
    lv_obj_set_style_text_font(diagnostics_arrow, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(diagnostics_arrow, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_text_align(diagnostics_arrow, LV_TEXT_ALIGN_CENTER, 0);
    log_ui_memory("settings ready");
}

static void build_diagnostics_screen(void)
{
    stop_pattern();
    single_test_status_label = NULL;
    single_test_action_label = NULL;
    single_test_running = false;
    sync_test_status_label = NULL;
    sync_test_action_label = NULL;
    lesson_selected = false;
    stop_diagnostics_updates();
    log_ui_memory("before diagnostics cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;

    make_label(ui_Screen1, "Aiuto-Vista", 10, 12, &lv_font_montserrat_16, 0xFFFFFF);
    make_label(ui_Screen1, "DIAGNOSE", 100, 13,
               &lv_font_montserrat_10, 0x00A0FF);

    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 262, 6);
    lv_obj_set_size(home_button, 50, 26);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(home_button, 4, 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, settings_home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_16, 0);
    lv_obj_center(home_symbol);
    lv_obj_t *tests_button = make_button(ui_Screen1, "TESTS", 222, 8, 46, 18,
                                         0x00A0FF, hardware_tests_open_event, NULL);
    lv_obj_set_style_radius(tests_button, 4, 0);
    lv_obj_t *software_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(software_card, 10, UI_CONTENT_TOP);
    lv_obj_set_size(software_card, 145, 180);
    lv_obj_clear_flag(software_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(software_card, 5, 0);
    lv_obj_set_style_bg_color(software_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(software_card, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(software_card, 1, 0);
    lv_obj_set_style_pad_all(software_card, 0, 0);
    make_label(software_card, "SOFTWARE & HARDWARE", 8, 6,
               &lv_font_montserrat_10, 0x00A0FF);
    make_divider(software_card, 8, 18, 124);

    const esp_app_desc_t *app = esp_app_get_description();
    lv_obj_t *software_keys = make_label(
        software_card,
        "Firmware:\nESP-IDF:\nTarget:\nAusgänge:\nCH1:\nCH2:",
        8, 25, &lv_font_montserrat_10, 0x808080);
    lv_obj_set_style_text_line_space(software_keys, 3, 0);
    char software_text[128];
    snprintf(software_text, sizeof(software_text),
             "%s\n%s\n%s\n%s\nGPIO26\nGPIO27",
             AIUTO_VISTA_FIRMWARE_VERSION, app->idf_ver,
             CONFIG_IDF_TARGET, hardware_ready ? "bereit" : "nicht bereit");
    lv_obj_t *software_values = make_label(software_card, software_text, 58, 25,
                                            &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_set_style_text_line_space(software_values, 3, 0);

    lv_obj_t *status_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(status_card, 165, UI_CONTENT_TOP);
    lv_obj_set_size(status_card, 145, 180);
    lv_obj_clear_flag(status_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(status_card, 5, 0);
    lv_obj_set_style_bg_color(status_card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(status_card, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(status_card, 1, 0);
    lv_obj_set_style_pad_all(status_card, 0, 0);
    make_label(status_card, "LAUFZEIT & SPEICHER", 8, 6,
               &lv_font_montserrat_10, 0x00A0FF);
    make_divider(status_card, 8, 18, 124);
    lv_obj_t *status_keys = make_label(
        status_card,
        "Resetgrund:\nLaufzeit:\nHeap min:\nIntern:\nLVGL frei:\nFragmentiert:\nStack:",
        8, 25, &lv_font_montserrat_10, 0x808080);
    lv_obj_set_style_text_line_space(status_keys, 4, 0);
    diagnostics_values_label = make_label(status_card, "", 70, 25,
                                           &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_set_width(diagnostics_values_label, 68);
    lv_obj_set_style_text_line_space(diagnostics_values_label, 4, 0);
    diagnostics_update(NULL);
    diagnostics_timer = lv_timer_create(diagnostics_update, 1000, NULL);
    if (!diagnostics_timer) ESP_LOGE(UI_TAG, "Creating diagnostics timer failed");
    log_ui_memory("diagnostics ready");
}

static lv_obj_t *make_hardware_test_card(int x, const char *title, const char *description,
                                         lv_obj_t **status_label, lv_obj_t **action_label,
                                         lv_event_cb_t toggle_event)
{
    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, x, UI_CONTENT_TOP);
    lv_obj_set_size(card, 145, 180);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(card, 5, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, title, 8, 6, &lv_font_montserrat_10, 0x00A0FF);
    make_divider(card, 8, 18, 124);
    lv_obj_t *description_label = make_label(card, description, 8, 25,
                                             &lv_font_montserrat_10, 0x808080);
    lv_obj_set_width(description_label, 124);
    lv_obj_set_style_text_line_space(description_label, 3, 0);

    *status_label = make_label(card, "GESTOPPT", 8, 130,
                               &lv_font_montserrat_10, 0x808080);
    lv_obj_set_width(*status_label, 124);
    lv_obj_t *action_button = make_button(card, "TEST STARTEN", 8, 148, 124, 21,
                                          0x00A0FF, toggle_event, NULL);
    *action_label = lv_obj_get_child(action_button, 0);
    return action_button;
}

static void build_hardware_tests_screen(void)
{
    stop_diagnostics_updates();
    stop_pattern();
    single_test_running = false;
    sync_test_running = false;
    lesson_selected = false;
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;

    make_label(ui_Screen1, "Aiuto-Vista", 10, 12, &lv_font_montserrat_16, 0xFFFFFF);
    make_label(ui_Screen1, "HARDWARETESTS", 100, 13,
               &lv_font_montserrat_10, 0x00A0FF);

    lv_obj_t *back_button = make_button(ui_Screen1, LV_SYMBOL_LEFT, 262, 6, 50, 26,
                                        0x000000, hardware_tests_back_event, NULL);
    lv_obj_set_style_radius(back_button, 4, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(back_button, 0), &lv_font_montserrat_16, 0);
    lv_obj_t *single_action_button = make_hardware_test_card(
        10, "1-KANAL-TEST",
        "CH1 / GPIO26\n"
        "1 kHz, 50 % Tastgrad\n"
        "0 bis 3,3 V gegen GND.\n"
        "Nach Stopp: GPIO26 LOW.",
        &single_test_status_label, &single_test_action_label, single_test_toggle_event);
    make_hardware_test_card(
        165, "2-KANAL-TEST",
        "CH1 / GPIO26: 1 kHz\n"
        "CH2 / GPIO27: 1 kHz\n"
        "Versatz: 100 µs\n"
        "Beide gegen denselben GND.",
        &sync_test_status_label, &sync_test_action_label, sync_test_toggle_event);
    (void)single_action_button;
    log_ui_memory("hardware tests ready");
}

static void build_scope_reset_screen(void)
{
    log_ui_memory("before reset-page cleanup");
    lv_obj_clean(ui_Screen1);
    confirm_overlay = NULL;
    timer_label = NULL;

    make_label(ui_Screen1, "Aiuto-Vista", 10, 12, &lv_font_montserrat_16, 0xFFFFFF);

    lv_obj_t *task_label = make_label(ui_Screen1, mode_name(), 0, 13,
                                      &lv_font_montserrat_10, 0x00A0FF);
    lv_obj_align(task_label, LV_ALIGN_TOP_MID, 0, 13);
    lv_obj_set_width(task_label, 150);
    lv_label_set_long_mode(task_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(task_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 262, 6);
    lv_obj_set_size(home_button, 50, 26);
    lv_obj_set_style_radius(home_button, 4, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_16, 0);
    lv_obj_center(home_symbol);

    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, 20, 52);
    lv_obj_set_size(card, 280, 170);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, "AUTO-VORBEREITUNG", 13, 11,
               &lv_font_montserrat_16, 0xFFFFFF);
    const char *auto_instruction = challenge.lesson && challenge.lesson->required_channels == 2 ?
        "Aktiviere CH1 und CH2 gleich\n"
        "skaliert, triggere auf CH1,\n"
        "dann AUTO am Oszilloskop." :
        "Drücke jetzt AUTO am\n"
        "Oszilloskop, um die Ansicht\n"
        "für die Messung vorzubereiten.";
    lv_obj_t *instruction = make_label(
        card, auto_instruction,
        13, 45, &lv_font_montserrat_10, 0x808080);
    lv_obj_set_style_text_line_space(instruction, 3, 0);

    lv_obj_t *button = make_button(card, "ZUR MESSAUFGABE", 140, 136, 103, 23,
                                   0x00A0FF, begin_question_event, NULL);
    lv_obj_set_style_radius(button, 5, 0);

    log_ui_memory("reset page ready");
    if (hardware_ready) start_scope_reset_signal();
}

static void build_question_screen(void)
{
    log_ui_memory("before question cleanup");
    lv_obj_clean(ui_Screen1);
    timer_label = NULL;
    action_button = NULL;
    action_label = NULL;
    all_values_button = NULL;
    all_values_label = NULL;
    for (uint8_t i = 0; i < SCOPEBUDDY_MAX_MEASUREMENTS; ++i) {
        measurement_values[i] = NULL;
        measurement_boxes[i] = NULL;
        measurement_marks[i] = NULL;
        measurement_selected[i] = false;
        measurement_revealed[i] = false;
    }
    elapsed_seconds = 0;
    char heading[80];
    snprintf(heading, sizeof(heading), "AUFGABE %lu", (unsigned long)question_number);
    lv_obj_t *home_button = lv_btn_create(ui_Screen1);
    lv_obj_set_pos(home_button, 262, 6);
    lv_obj_set_size(home_button, 50, 26);
    lv_obj_set_style_radius(home_button, 4, 0);
    lv_obj_set_style_bg_color(home_button, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(home_button, lv_color_hex(0x00A0FF), 0);
    lv_obj_set_style_border_width(home_button, 1, 0);
    lv_obj_add_event_cb(home_button, home_event, LV_EVENT_RELEASED, NULL);
    lv_obj_t *home_symbol = lv_label_create(home_button);
    lv_label_set_text(home_symbol, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(home_symbol, &lv_font_montserrat_16, 0);
    lv_obj_center(home_symbol);

    make_label(ui_Screen1, heading, 10, 13, &lv_font_montserrat_10, 0x00A0FF);
    lv_obj_t *task_label = make_label(ui_Screen1, mode_name(), 0, 13,
                                      &lv_font_montserrat_10, 0x808080);
    lv_obj_align(task_label, LV_ALIGN_TOP_MID, 0, 13);
    lv_obj_set_width(task_label, 150);
    lv_label_set_long_mode(task_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(task_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *setup_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(setup_card, 10, UI_CONTENT_TOP);
    lv_obj_set_size(setup_card, 145, 162);
    lv_obj_clear_flag(setup_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(setup_card, 0, 0);
    lv_obj_set_style_bg_opa(setup_card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(setup_card, 0, 0);
    lv_obj_set_style_pad_all(setup_card, 0, 0);
    make_label(setup_card, "MESSAUFBAU", 10, 6,
               &lv_font_montserrat_10, 0x00A0FF);

    const char *setup_texts[] = {
        challenge.context,
        challenge.lesson->connection_hint,
        challenge.lesson->trigger_hint,
    };
    const uint32_t setup_colors[] = { 0x808080, 0x00FFFF, 0xFFFF00 };
    int setup_y = 22;
    for (size_t i = 0; i < 3U; ++i) {
        lv_obj_t *text = make_label(setup_card, setup_texts[i], 10, setup_y,
                                    &lv_font_montserrat_10, setup_colors[i]);
        lv_obj_set_width(text, 135);
        lv_obj_set_style_text_line_space(text, 1, 0);
        lv_obj_update_layout(text);
        setup_y += lv_obj_get_height(text) + 4;
    }

    lv_obj_t *separator_l = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(separator_l, 158, UI_CONTENT_TOP);
    lv_obj_set_size(separator_l, 1, 162);
    lv_obj_clear_flag(separator_l, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(separator_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(separator_l, 0, 0);
    lv_obj_t *separator_r = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(separator_r, 162, UI_CONTENT_TOP);
    lv_obj_set_size(separator_r, 1, 162);
    lv_obj_clear_flag(separator_r, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(separator_r, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(separator_r, 0, 0);

    lv_obj_t *measurements_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(measurements_card, 165, UI_CONTENT_TOP);
    lv_obj_set_size(measurements_card, 145, 162);
    lv_obj_clear_flag(measurements_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(measurements_card, 0, 0);
    lv_obj_set_style_bg_opa(measurements_card, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(measurements_card, 0, 0);
    lv_obj_set_style_pad_all(measurements_card, 0, 0);
    make_label(measurements_card, "MESSWERTE", 7, 6,
               &lv_font_montserrat_10, 0x00A0FF);
    for (uint8_t i = 0; i < challenge.measurement_count; ++i) {
        make_measurement_item(measurements_card, &challenge.measurements[i], 23 + i * 40, i);
    }

    if (!setting_reveal_values) {
        all_values_button = make_button(ui_Screen1, "ALLE WERTE\nANZEIGEN", 10, 210, 94, 24,
                                        0x00A0FF, solve_all_event, NULL);
        lv_obj_set_style_radius(all_values_button, 5, 0);
        lv_obj_set_style_bg_opa(all_values_button, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(all_values_button, lv_color_hex(0x00A0FF), 0);
        lv_obj_set_style_border_width(all_values_button, 2, 0);
        lv_obj_set_style_shadow_width(all_values_button, 0, 0);
        all_values_label = lv_obj_get_child(all_values_button, 0);
        lv_obj_set_style_text_color(all_values_label, lv_color_hex(0xFFFFFF), 0);
        action_button = make_button(ui_Screen1, "AUSGEWÄHLTE WERTE\nANZEIGEN", 113, 210, 94, 24,
                                    0x00A0FF, solve_event, NULL);
        lv_obj_set_style_radius(action_button, 5, 0);
        lv_obj_set_style_bg_opa(action_button, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(action_button, lv_color_hex(0x00A0FF), 0);
        lv_obj_set_style_border_width(action_button, 2, 0);
        lv_obj_set_style_shadow_width(action_button, 0, 0);
        action_label = lv_obj_get_child(action_button, 0);
        lv_obj_set_style_text_color(action_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_add_state(action_button, LV_STATE_DISABLED);
        lv_obj_set_style_border_color(action_button, lv_color_hex(0x404040), LV_STATE_DISABLED);
        lv_obj_set_style_text_color(action_label, lv_color_hex(0x808080), LV_STATE_DISABLED);
        update_solution_buttons();
    }
    lv_obj_t *advance_button = make_button(ui_Screen1, "NÄCHSTE\nMESSAUFGABE",
                                           216, 210, 94, 24, 0x00A0FF, advance_event, NULL);
    lv_obj_set_style_radius(advance_button, 5, 0);

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
    log_operation_error("Starting GPIO26 output", gpio_wave_start());
}
void LedOff(lv_event_t *e)
{
    (void)e;
    log_operation_error("Stopping GPIO26 output", gpio_wave_stop());
}
void WaveFrequencyChanged(lv_event_t *e) { (void)e; }
void WaveDutyChanged(lv_event_t *e) { (void)e; }
void WaveRandomChallenge(lv_event_t *e) { (void)e; }
void WaveRevealChallenge(lv_event_t *e) { (void)e; }
void WaveUiInit(void) { }
