#include "encoder_input.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define ENCODER_TAG              "ENCODER"
#define BUTTON_DEBOUNCE_READS    2

static volatile int32_t encoder_steps;
static volatile uint8_t previous_encoder_state;
static volatile int8_t encoder_quarter_steps;
static lv_indev_drv_t encoder_indev_driver;
static lv_indev_t *encoder_indev;
static lv_group_t *encoder_group;
static lv_group_t *encoder_active_group;
static lv_style_t encoder_focus_style;
static bool encoder_enabled = true;
static bool encoder_is_active_input = true;

static void update_focus_outline(bool show)
{
    lv_group_t *group = encoder_active_group ? encoder_active_group : encoder_group;
    if (group == NULL) return;

    lv_obj_t *focused = lv_group_get_focused(group);
    if (focused == NULL || !lv_obj_is_valid(focused)) return;

    const lv_style_selector_t selector = LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY;
    lv_obj_remove_style(focused, &encoder_focus_style, selector);
    if (show && encoder_enabled) {
        lv_obj_add_style(focused, &encoder_focus_style, selector);
    }
    lv_obj_invalidate(focused);
}

static void encoder_focus_changed(lv_group_t *group)
{
    lv_obj_t *focused = lv_group_get_focused(group);
    if (focused == NULL) return;

    const lv_style_selector_t selector = LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY;
    lv_obj_remove_style(focused, &encoder_focus_style, selector);
    if (encoder_enabled && encoder_is_active_input) {
        lv_obj_add_style(focused, &encoder_focus_style, selector);
    }
}

void encoder_input_activate_group(lv_group_t *group)
{
    if (encoder_indev == NULL || group == NULL) return;

    __atomic_store_n(&encoder_steps, 0, __ATOMIC_RELAXED);
    encoder_active_group = group;
    lv_group_set_focus_cb(group, encoder_focus_changed);
    if (encoder_enabled) lv_indev_set_group(encoder_indev, group);
}

void encoder_input_set_enabled(bool enabled)
{
    encoder_enabled = enabled;
    if (encoder_indev == NULL || encoder_group == NULL) return;

    lv_group_t *active_group = encoder_active_group ? encoder_active_group : encoder_group;
    lv_obj_t *focused = lv_group_get_focused(active_group);
    const lv_style_selector_t selector = LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY;

    if (!enabled) {
        __atomic_store_n(&encoder_steps, 0, __ATOMIC_RELAXED);
        lv_indev_set_group(encoder_indev, NULL);
        if (focused) {
            lv_obj_remove_style(focused, &encoder_focus_style, selector);
            lv_obj_invalidate(focused);
        }
    } else {
        lv_indev_set_group(encoder_indev, active_group);
        if (focused && encoder_is_active_input) {
            lv_obj_remove_style(focused, &encoder_focus_style, selector);
            lv_obj_add_style(focused, &encoder_focus_style, selector);
            lv_obj_invalidate(focused);
        }
    }
}

void encoder_input_touch_event(lv_event_t *event)
{
    (void)event;
    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) return;

    encoder_is_active_input = false;
    update_focus_outline(false);
}

/* Valid Gray-code transitions. Clockwise and counter-clockwise transitions
 * have opposite signs; bouncing and invalid two-bit jumps contribute zero. */
static const int8_t transition_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};

static void encoder_gpio_isr(void *arg)
{
    (void)arg;
    const uint8_t current = (gpio_get_level(ENCODER_GPIO_CLK) << 1) |
                            gpio_get_level(ENCODER_GPIO_DT);
    const uint8_t previous = previous_encoder_state;

    if (current != previous) {
        encoder_quarter_steps += transition_table[(previous << 2) | current];
        previous_encoder_state = current;

        if (encoder_quarter_steps >= 4) {
            __atomic_add_fetch(&encoder_steps, 1, __ATOMIC_RELAXED);
            encoder_quarter_steps = 0;
        } else if (encoder_quarter_steps <= -4) {
            __atomic_sub_fetch(&encoder_steps, 1, __ATOMIC_RELAXED);
            encoder_quarter_steps = 0;
        }
    }
}

static void encoder_read(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    (void)driver;
    static bool last_button_sample;
    static bool stable_button;
    static uint8_t stable_count;
    int32_t steps = __atomic_exchange_n(&encoder_steps, 0, __ATOMIC_RELAXED);
    const bool button_sample = gpio_get_level(ENCODER_GPIO_SW) == 0;

    if (steps > INT16_MAX) steps = INT16_MAX;
    if (steps < INT16_MIN) steps = INT16_MIN;

    if (button_sample == last_button_sample) {
        if (stable_count < BUTTON_DEBOUNCE_READS) ++stable_count;
        if (stable_count == BUTTON_DEBOUNCE_READS) stable_button = button_sample;
    } else {
        last_button_sample = button_sample;
        stable_count = 1;
    }

    data->enc_diff = (int16_t)steps;
    data->state = stable_button
                      ? LV_INDEV_STATE_PRESSED
                      : LV_INDEV_STATE_RELEASED;

    if (encoder_enabled && (steps != 0 || stable_button)) {
        encoder_is_active_input = true;
        update_focus_outline(true);
    }
}

esp_err_t encoder_input_init(void)
{
    const gpio_config_t input_config = {
        .pin_bit_mask = (1ULL << ENCODER_GPIO_CLK) |
                        (1ULL << ENCODER_GPIO_DT) |
                        (1ULL << ENCODER_GPIO_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&input_config);
    if (err != ESP_OK) return err;

    previous_encoder_state = (gpio_get_level(ENCODER_GPIO_CLK) << 1) |
                             gpio_get_level(ENCODER_GPIO_DT);
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = gpio_set_intr_type(ENCODER_GPIO_CLK, GPIO_INTR_ANYEDGE);
    if (err != ESP_OK) return err;
    err = gpio_set_intr_type(ENCODER_GPIO_DT, GPIO_INTR_ANYEDGE);
    if (err != ESP_OK) return err;
    err = gpio_isr_handler_add(ENCODER_GPIO_CLK, encoder_gpio_isr, NULL);
    if (err != ESP_OK) return err;
    err = gpio_isr_handler_add(ENCODER_GPIO_DT, encoder_gpio_isr, NULL);
    if (err != ESP_OK) {
        gpio_isr_handler_remove(ENCODER_GPIO_CLK);
        return err;
    }

    if (!lvgl_port_lock(0)) return ESP_ERR_TIMEOUT;
    lv_style_init(&encoder_focus_style);
    lv_style_set_outline_color(&encoder_focus_style, lv_color_white());
    lv_style_set_outline_width(&encoder_focus_style, 4);
    lv_style_set_outline_pad(&encoder_focus_style, 3);
    lv_style_set_outline_opa(&encoder_focus_style, LV_OPA_COVER);

    encoder_group = lv_group_create();
    if (encoder_group == NULL) {
        lvgl_port_unlock();
        return ESP_ERR_NO_MEM;
    }
    lv_group_set_default(encoder_group);
    encoder_active_group = encoder_group;
    lv_group_set_focus_cb(encoder_group, encoder_focus_changed);

    lv_indev_drv_init(&encoder_indev_driver);
    encoder_indev_driver.type = LV_INDEV_TYPE_ENCODER;
    encoder_indev_driver.read_cb = encoder_read;
    encoder_indev = lv_indev_drv_register(&encoder_indev_driver);
    if (encoder_indev == NULL) {
        lv_group_del(encoder_group);
        encoder_group = NULL;
        lvgl_port_unlock();
        return ESP_ERR_NO_MEM;
    }
    lv_indev_set_group(encoder_indev, encoder_group);
    lvgl_port_unlock();

    ESP_LOGI(ENCODER_TAG, "Ready: CLK=GPIO%d, DT=GPIO%d, SW=GPIO%d",
             ENCODER_GPIO_CLK, ENCODER_GPIO_DT, ENCODER_GPIO_SW);
    return ESP_OK;
}
