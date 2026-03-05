#include "drv_inputs.h"

#include "drv_gpio.h"
#include "drv_timebase.h"

#define INPUT_DEBOUNCE_MS 5U

static uint32_t g_last_sample = 0;
static uint32_t g_stable_state = 0;
static uint32_t g_prev_stable_state = 0;
static uint32_t g_last_change_ms = 0;

void drv_inputs_init(void) {
    g_last_sample = drv_gpio_read_inputs_raw();
    g_stable_state = g_last_sample;
    g_prev_stable_state = g_last_sample;
    g_last_change_ms = drv_timebase_millis();
}

void drv_inputs_poll(void) {
    const uint32_t now = drv_timebase_millis();
    const uint32_t sample = drv_gpio_read_inputs_raw();

    if (sample != g_last_sample) {
        g_last_sample = sample;
        g_last_change_ms = now;
        return;
    }

    if ((int32_t)(now - g_last_change_ms) >= (int32_t)INPUT_DEBOUNCE_MS) {
        g_stable_state = sample;
    }
}

drv_inputs_snapshot_t drv_inputs_get(void) {
    drv_inputs_snapshot_t out;
    out.state = g_stable_state;
    out.rising_edges = (~g_prev_stable_state) & g_stable_state;
    g_prev_stable_state = g_stable_state;
    return out;
}
