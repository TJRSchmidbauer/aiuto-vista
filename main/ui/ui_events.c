#include "ui.h"
#include "bsp_extra.h"
#include "encoder_input.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>

#define SCOPEBUDDY_FIRMWARE_VERSION "0.3"
#define UI_TAG "SCOPEBUDDY_UI"

LV_FONT_DECLARE(scopebuddy_font_14);
LV_FONT_DECLARE(scopebuddy_font_24);

typedef enum {
    GAME_NONE,
    GAME_EASY,
    GAME_MEDIUM,
    GAME_HARD,
} game_mode_t;

typedef struct {
    uint32_t frequency_hz;
    uint8_t duty_percent;
} signal_state_t;

typedef struct {
    signal_state_t state_a;
    signal_state_t state_b;
    uint16_t pause_ms;
    uint16_t state_duration_ms;
    uint8_t pulse_count;
} challenge_t;

static bool hardware_ready;
static bool alternating_state_b;
static uint8_t question_number;
static game_mode_t game_mode;
static challenge_t challenge;
static esp_timer_handle_t pattern_timer;
static StaticSemaphore_t pattern_mutex_buffer;
static SemaphoreHandle_t pattern_mutex;
static bool pattern_enabled;
static uint32_t recent_signatures[18];
static uint8_t recent_count;

static lv_obj_t *action_button;
static lv_obj_t *action_label;
static lv_obj_t *all_values_button;
static lv_obj_t *all_values_label;
static lv_obj_t *advance_button;
static lv_obj_t *timer_label;
static lv_timer_t *game_clock;
static lv_timer_t *splash_timer;
static uint32_t elapsed_seconds;
static lv_obj_t *measurement_values[5];
static lv_obj_t *measurement_boxes[5];
static lv_obj_t *measurement_marks[5];
static lv_obj_t *confirm_overlay;
static lv_group_t *confirm_group;
static lv_group_t *confirm_background_group;
static lv_obj_t *page_default_focus;
static bool measurement_selected[5];
static bool measurement_revealed[5];
static bool setting_show_timer = true;
static bool setting_reveal_values;
static bool setting_scope_reset = true;
static bool setting_encoder_enabled = true;
static bool splash_active;

static void mode_event(lv_event_t *event);
static void build_splash_screen(void);
static void build_start_screen(void);
static void build_settings_screen(void);
static void build_scope_reset_screen(void);
static void build_question_screen(void);

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
    if (game_mode == GAME_NONE || !timer_label) return;
    ++elapsed_seconds;
    lv_label_set_text_fmt(timer_label, "%02lu:%02lu",
                          (unsigned long)(elapsed_seconds / 60U),
                          (unsigned long)(elapsed_seconds % 60U));
}

static uint32_t random_range(uint32_t minimum, uint32_t maximum)
{
    return minimum + (esp_random() % (maximum - minimum + 1U));
}

static const char *mode_name(void)
{
    if (game_mode == GAME_EASY) return "LEICHT";
    if (game_mode == GAME_MEDIUM) return "MITTEL";
    return "SCHWER";
}

static uint32_t challenge_signature(void)
{
    uint32_t value = challenge.state_a.frequency_hz * 2654435761U;
    value ^= ((uint32_t)challenge.state_a.duty_percent << 24);
    value ^= ((uint32_t)challenge.pulse_count << 16);
    value ^= challenge.state_b.frequency_hz * 2246822519U;
    value ^= ((uint32_t)challenge.state_b.duty_percent << 8);
    value ^= challenge.pause_ms | ((uint32_t)challenge.state_duration_ms << 16);
    return value;
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
    log_operation_error("Stopping burst output", gpio_burst_stop());
    log_operation_error("Stopping GPIO48 output", gpio_wave_stop());
}

static void stop_pattern(void)
{
    if (pattern_mutex == NULL) {
        log_operation_error("Stopping GPIO48 output", gpio_wave_stop());
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

    if (game_mode == GAME_HARD) {
        alternating_state_b = !alternating_state_b;
        esp_err_t err = gpio_wave_set_frequency(
            alternating_state_b ? challenge.state_b.frequency_hz : challenge.state_a.frequency_hz);
        if (err == ESP_OK) {
            err = gpio_wave_set_duty(alternating_state_b ? challenge.state_b.duty_percent :
                                                            challenge.state_a.duty_percent);
        }
        if (err == ESP_OK) {
            schedule_pattern_locked((uint64_t)challenge.state_duration_ms * 1000ULL);
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
    uint32_t signature;
    do {
        challenge = (challenge_t){0};
        if (game_mode == GAME_EASY) {
            challenge.state_a.frequency_hz = random_range(50, 10000);
            challenge.state_a.duty_percent = random_range(10, 90);
        } else if (game_mode == GAME_MEDIUM) {
            challenge.state_a.frequency_hz = random_range(100, 2000);
            challenge.state_a.duty_percent = random_range(15, 85);
            challenge.pulse_count = random_range(3, 15);
            challenge.pause_ms = random_range(10, 100);
        } else {
            challenge.state_a.frequency_hz = random_range(100, 8000);
            do challenge.state_b.frequency_hz = random_range(100, 8000);
            while (challenge.state_b.frequency_hz > challenge.state_a.frequency_hz * 4U / 5U &&
                   challenge.state_b.frequency_hz < challenge.state_a.frequency_hz * 6U / 5U);
            challenge.state_a.duty_percent = random_range(10, 90);
            challenge.state_b.duty_percent = random_range(10, 90);
            challenge.state_duration_ms = random_range(150, 1200);
        }
        signature = challenge_signature();
    } while (signature_seen(signature));
    remember_signature(signature);
}

static void start_challenge_signal(void)
{
    if (pattern_mutex == NULL) {
        ESP_LOGE(UI_TAG, "Cannot start challenge: signal mutex unavailable");
        return;
    }
    xSemaphoreTake(pattern_mutex, portMAX_DELAY);
    stop_pattern_locked();

    esp_err_t err;
    if (game_mode == GAME_MEDIUM) {
        err = gpio_burst_start(challenge.state_a.frequency_hz,
                               challenge.state_a.duty_percent,
                               challenge.pulse_count,
                               challenge.pause_ms);
    } else {
        err = gpio_wave_set_frequency(challenge.state_a.frequency_hz);
        if (err == ESP_OK) err = gpio_wave_set_duty(challenge.state_a.duty_percent);
        if (err == ESP_OK) err = gpio_wave_start();
    }
    if (err != ESP_OK) {
        log_operation_error("Starting challenge output", err);
        xSemaphoreGive(pattern_mutex);
        return;
    }

    if (game_mode == GAME_HARD) {
        pattern_enabled = true;
        alternating_state_b = false;
        schedule_pattern_locked((uint64_t)challenge.state_duration_ms * 1000ULL);
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

    /* Use the safe extremes supported by the GPIO waveform driver. The choice
       is based on the signal state shown first in the upcoming challenge so
       AUTO leaves the horizontal timebase as far away from it as possible. */
    uint32_t reset_frequency =
        (challenge.state_a.frequency_hz < 448U) ? 20000U : 10U;
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
    game_mode = GAME_NONE;
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
    for (uint8_t i = 0; i < 5; ++i) {
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

static void make_measurement_item(lv_obj_t *parent, const char *text, int y, uint8_t index)
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
    make_label(parent, text, 70, y + 3, &lv_font_montserrat_14, 0xDCE8F7);
    measurement_values[index] = make_label(parent, "---", 306, y + 3,
                                            &lv_font_montserrat_14, 0x607895);

    lv_obj_t *touch_area = lv_obj_create(parent);
    lv_obj_set_pos(touch_area, 18, y - 5);
    lv_obj_set_size(touch_area, 370, 35);
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

static void make_mode_card(int x, game_mode_t mode, const char *title, const char *subtitle,
                           const char *details, uint32_t accent)
{
    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(card, x, 122);
    lv_obj_set_size(card, 230, 278);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(accent), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 0, 0);

    make_label(card, title, 18, 20, &lv_font_montserrat_24, accent);
    make_label(card, subtitle, 18, 58, &lv_font_montserrat_14, 0xDCE8F7);
    lv_obj_t *description = make_label(card, details, 18, 98,
                                       &lv_font_montserrat_14, 0x8FA5C2);
    lv_obj_set_width(description, 194);
    lv_obj_set_style_text_line_space(description, 6, 0);
    make_label(card, "3 MESSAUFGABEN", 18, 190, &lv_font_montserrat_14, 0x607895);
    make_button(card, "STARTEN", 18, 222, 194, 42, accent,
                mode_event, (void *)(uintptr_t)mode);
}

static void mode_event(lv_event_t *event)
{
    game_mode = (game_mode_t)(uintptr_t)lv_event_get_user_data(event);
    question_number = 1;
    queue_ui_action(prepare_question_async, "prepare question");
}

static void reveal_selected_value(uint8_t index)
{
    char value[80];
    double period_us = 1000000.0 / challenge.state_a.frequency_hz;
    if (game_mode == GAME_EASY) {
        if (index == 0) {
            snprintf(value, sizeof(value), "%lu Hz", (unsigned long)challenge.state_a.frequency_hz);
        } else if (index == 1) {
            snprintf(value, sizeof(value), "%u %%", challenge.state_a.duty_percent);
        } else if (index == 2) {
            snprintf(value, sizeof(value), "%.2f µs", period_us);
        } else if (index == 3) {
            snprintf(value, sizeof(value), "ca. 3.30 V");
        } else {
            snprintf(value, sizeof(value), "ca. %.2f V",
                     3.3 * challenge.state_a.duty_percent / 100.0);
        }
    } else if (game_mode == GAME_MEDIUM) {
        uint32_t rmt_period_us = 0;
        uint32_t high_time_us = 0;
        esp_err_t timing_err = gpio_burst_get_timing_us(challenge.state_a.frequency_hz,
                                                        challenge.state_a.duty_percent,
                                                        &rmt_period_us,
                                                        &high_time_us);
        if (timing_err != ESP_OK) {
            reveal_measurement(index, "FEHLER");
            log_operation_error("Calculating displayed burst timing", timing_err);
            return;
        }
        double actual_frequency_hz = 1000000.0 / rmt_period_us;
        double actual_duty_percent = 100.0 * high_time_us / rmt_period_us;
        double burst_duration_ms =
            ((double)(challenge.pulse_count - 1U) * rmt_period_us + high_time_us) / 1000.0;
        if (index == 0) {
            snprintf(value, sizeof(value), "%.2f Hz", actual_frequency_hz);
        } else if (index == 1) {
            snprintf(value, sizeof(value), "%.2f %%", actual_duty_percent);
        } else if (index == 2) {
            snprintf(value, sizeof(value), "%u", challenge.pulse_count);
        } else if (index == 3) {
            snprintf(value, sizeof(value), "%.3f ms", burst_duration_ms);
        } else {
            snprintf(value, sizeof(value), "%u ms", challenge.pause_ms);
        }
    } else {
        if (index == 0) {
            snprintf(value, sizeof(value), "%lu Hz", (unsigned long)challenge.state_a.frequency_hz);
        } else if (index == 1) {
            snprintf(value, sizeof(value), "%u %%", challenge.state_a.duty_percent);
        } else if (index == 2) {
            snprintf(value, sizeof(value), "%lu Hz", (unsigned long)challenge.state_b.frequency_hz);
        } else if (index == 3) {
            snprintf(value, sizeof(value), "%u %%", challenge.state_b.duty_percent);
        } else {
            snprintf(value, sizeof(value), "%u ms", challenge.state_duration_ms);
        }
    }
    reveal_measurement(index, value);
}

static void solve_event(lv_event_t *event)
{
    (void)event;
    bool hide_selected = true;
    for (uint8_t i = 0; i < 5; ++i) {
        if (measurement_selected[i] && !measurement_revealed[i]) hide_selected = false;
    }
    for (uint8_t i = 0; i < 5; ++i) {
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
    for (uint8_t i = 0; i < 5; ++i) {
        if (!measurement_revealed[i]) hide_all = false;
    }
    for (uint8_t i = 0; i < 5; ++i) {
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

static void timer_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_show_timer = lv_obj_has_state(toggle, LV_STATE_CHECKED);
}

static void values_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_reveal_values = lv_obj_has_state(toggle, LV_STATE_CHECKED);
}

static void scope_reset_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_scope_reset = lv_obj_has_state(toggle, LV_STATE_CHECKED);
}

static void encoder_setting_event(lv_event_t *event)
{
    lv_obj_t *toggle = lv_event_get_target(event);
    setting_encoder_enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    encoder_input_set_enabled(setting_encoder_enabled);
}

static void make_setting_row(const char *title, const char *description, int y,
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
}

static void build_start_screen(void)
{
    if (splash_timer) {
        lv_timer_del(splash_timer);
        splash_timer = NULL;
    }
    splash_active = false;
    stop_pattern();
    game_mode = GAME_NONE;
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

    make_mode_card(25, GAME_EASY, "LEICHT", "GRUNDLAGEN",
                   "Kontinuierliche Signale\nFrequenz, Tastgrad, Periode\nund Spannungswerte", 0x2684FF);
    make_mode_card(285, GAME_MEDIUM, "MITTEL", "BURSTSIGNALE",
                   "Exakte Impulspakete\nPulszahl, Burstdauer\nund Low-Pause", 0xD59A28);
    make_mode_card(545, GAME_HARD, "SCHWER", "WECHSELSIGNALE",
                   "Zwei Signalzustände\nWerte A und B sowie\ndie Zustandsdauer", 0xC14F71);
    make_label(ui_Screen1, "Das Messsignal wird an GPIO48 ausgegeben.",
               25, 438, &lv_font_montserrat_14, 0x2684FF);

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
    stop_pattern();
    game_mode = GAME_NONE;
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
                     "Zeigt alle Sollwerte sofort und entfernt die Lösungsbuttons.",
                     173, setting_reveal_values, values_setting_event);
    make_setting_row("AUTO-VORBEREITUNG",
                     "Gibt vor jeder Messaufgabe ein Referenzsignal für AUTO aus.",
                     245, setting_scope_reset, scope_reset_setting_event);
    make_setting_row("ENCODER AKTIVIEREN",
                     "Aktiviert Drehsteuerung und weiße Auswahlkontur.",
                     317, setting_encoder_enabled, encoder_setting_event);

    lv_obj_t *version_card = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(version_card, 25, 397);
    lv_obj_set_size(version_card, 750, 50);
    lv_obj_clear_flag(version_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(version_card, 10, 0);
    lv_obj_set_style_bg_color(version_card, lv_color_hex(0x0D1927), 0);
    lv_obj_set_style_border_color(version_card, lv_color_hex(0x26384B), 0);
    lv_obj_set_style_border_width(version_card, 1, 0);
    lv_obj_set_style_pad_all(version_card, 0, 0);
    make_label(version_card, "FIRMWARE-VERSION", 22, 14,
               &lv_font_montserrat_14, 0xAFC2DC);
    make_label(version_card, SCOPEBUDDY_FIRMWARE_VERSION, 680, 8,
               &lv_font_montserrat_24, 0x2684FF);
    log_ui_memory("settings ready");
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
    snprintf(heading, sizeof(heading), "MESSAUFGABE %u / 3", question_number);
    make_label(ui_Screen1, heading, 250, 32, &lv_font_montserrat_14, 0x2684FF);
    make_label(ui_Screen1, mode_name(), 630, 32,
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
    lv_obj_t *instruction = make_label(
        card,
        "Drücke jetzt den AUTO-Button am Oszilloskop,\num die Ansicht für die Messung vorzubereiten.",
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
    for (uint8_t i = 0; i < 5; ++i) {
        measurement_values[i] = NULL;
        measurement_boxes[i] = NULL;
        measurement_marks[i] = NULL;
        measurement_selected[i] = false;
        measurement_revealed[i] = false;
    }
    elapsed_seconds = 0;
    char heading[80];
    snprintf(heading, sizeof(heading), "MESSAUFGABE %u / 3", question_number);
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
    make_label(ui_Screen1, mode_name(), setting_show_timer ? 545 : 630, 32,
               &lv_font_montserrat_14, 0x8FA5C2);
    if (setting_show_timer) {
        timer_label = make_label(ui_Screen1, "00:00", 630, 26,
                                 &lv_font_montserrat_24, 0xDCE8F7);
    }
    make_divider(ui_Screen1, 25, 96, 750);

    make_label(ui_Screen1, "BESTIMME FOLGENDE WERTE:", 28, 116,
               &lv_font_montserrat_14, 0xDCE8F7);
    if (game_mode == GAME_EASY) {
        make_measurement_item(ui_Screen1, "Frequenz", 151, 0);
        make_measurement_item(ui_Screen1, "Tastgrad", 188, 1);
        make_measurement_item(ui_Screen1, "Periodendauer", 225, 2);
        make_measurement_item(ui_Screen1, "Spitze-Spitze-Spannung (Vpp)", 262, 3);
        make_measurement_item(ui_Screen1, "DC-Mittelwert", 299, 4);
    } else if (game_mode == GAME_MEDIUM) {
        make_measurement_item(ui_Screen1, "Frequenz", 151, 0);
        make_measurement_item(ui_Screen1, "Tastgrad", 188, 1);
        make_measurement_item(ui_Screen1, "Pulse je Burst", 225, 2);
        make_measurement_item(ui_Screen1, "Burstdauer (Flanke-Flanke)", 262, 3);
        make_measurement_item(ui_Screen1, "Low-Pause zwischen Bursts", 299, 4);
    } else {
        make_measurement_item(ui_Screen1, "Frequenz A", 151, 0);
        make_measurement_item(ui_Screen1, "Tastgrad A", 188, 1);
        make_measurement_item(ui_Screen1, "Frequenz B", 225, 2);
        make_measurement_item(ui_Screen1, "Tastgrad B", 262, 3);
        make_measurement_item(ui_Screen1, "Zustandsdauer A/B", 299, 4);
    }

    make_divider(ui_Screen1, 25, 346, 750);
    if (setting_reveal_values) {
        for (uint8_t i = 0; i < 5; ++i) reveal_selected_value(i);
    } else {
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
}

void GameUiBuildStart(void)
{
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
    if (game_mode == GAME_NONE && !splash_active) build_start_screen();
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
