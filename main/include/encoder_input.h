#ifndef ENCODER_INPUT_H
#define ENCODER_INPUT_H

#include "esp_err.h"
#include "lvgl.h"

/*
 * CrowPanel 5" GPIO header connections. These pins are shared with the
 * optional radio module, so that module must be disabled when the encoder is
 * connected.
 */
#define ENCODER_GPIO_CLK 29
#define ENCODER_GPIO_DT  30
#define ENCODER_GPIO_SW  32

/** Initialize the rotary encoder and register it as an LVGL input device. */
esp_err_t encoder_input_init(void);

/** Route encoder input to another LVGL group (for modal dialogs). */
void encoder_input_activate_group(lv_group_t *group);

/** Enable or disable encoder navigation and its white focus outline. */
void encoder_input_set_enabled(bool enabled);

/** Hide the encoder outline when an interactive object is operated by touch. */
void encoder_input_touch_event(lv_event_t *event);

#endif
