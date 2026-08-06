/* Signal generation API for the measurement tasks.
 * Fork of ScopeBuddy (https://github.com/johannesboernsen/ScopeBuddy). */
#ifndef SCOPEBUDDY_OUTPUT_H
#define SCOPEBUDDY_OUTPUT_H

#include "scopebuddy_lessons.h"

esp_err_t scopebuddy_output_start(const scope_signal_spec_t *signal);
esp_err_t scopebuddy_output_stop(void);

#endif
