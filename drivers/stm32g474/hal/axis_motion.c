#include "axis_motion.h"

#include "../../../src/hal.h"
#include "../board/board_pins.h"

#include <string.h>
#include <stdlib.h>

static uint32_t compute_tick_divider(uint32_t step_period_us) {
    const uint32_t tick_period_us = 1000000u / BOARD_STEP_TICK_HZ;
    if (step_period_us <= tick_period_us) {
        return 1u;
    }
    const uint32_t rounded = (step_period_us + (tick_period_us / 2u)) / tick_period_us;
    return (rounded == 0u) ? 1u : rounded;
}

void axis_motion_init(axis_motion_state_t *state) {
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
}

bool axis_motion_start_xy(axis_motion_state_t *state,
                          int32_t x_steps,
                          int32_t y_steps,
                          bool x_dir_positive,
                          bool y_dir_positive,
                          uint32_t step_period_us) {
    if (!state) {
        return false;
    }

    if (state->active) {
        return false;
    }

    const uint32_t abs_x = (uint32_t)labs(x_steps);
    const uint32_t abs_y = (uint32_t)labs(y_steps);
    if (abs_x == 0u && abs_y == 0u) {
        return true;
    }

    hal_stepper_enable(true);
    hal_stepper_set_dir(HAL_AXIS_X, x_dir_positive);
    hal_stepper_set_dir(HAL_AXIS_Y, y_dir_positive);

    state->aborted = false;
    state->tick_divider = compute_tick_divider(step_period_us);
    state->tick_counter = 0u;

    if (abs_x >= abs_y) {
        state->primary_axis_mask = (1u << HAL_AXIS_X);
        state->minor_axis_mask = (1u << HAL_AXIS_Y);
        state->primary_steps_total = abs_x;
        state->minor_steps_total = abs_y;
    } else {
        state->primary_axis_mask = (1u << HAL_AXIS_Y);
        state->minor_axis_mask = (1u << HAL_AXIS_X);
        state->primary_steps_total = abs_y;
        state->minor_steps_total = abs_x;
    }

    state->remaining_primary_steps = state->primary_steps_total;
    state->error_accum = state->primary_steps_total / 2u;
    state->active = true;
    return true;
}

uint32_t axis_motion_step_tick(axis_motion_state_t *state) {
    if (!state || !state->active || state->aborted) {
        return 0u;
    }

    state->tick_counter++;
    if (state->tick_counter < state->tick_divider) {
        return 0u;
    }
    state->tick_counter = 0u;

    if (state->remaining_primary_steps == 0u) {
        state->active = false;
        return 0u;
    }

    uint32_t step_mask = state->primary_axis_mask;
    state->error_accum += state->minor_steps_total;
    if (state->minor_steps_total > 0u && state->error_accum >= state->primary_steps_total) {
        step_mask |= state->minor_axis_mask;
        state->error_accum -= state->primary_steps_total;
    }

    state->remaining_primary_steps--;
    if (state->remaining_primary_steps == 0u) {
        state->active = false;
    }

    return step_mask;
}

bool axis_motion_wait_complete(axis_motion_state_t *state) {
    if (!state) {
        return false;
    }

    while (state->active) {
        hal_poll();
        hal_inputs_t inputs;
        hal_read_inputs(&inputs);
        if (inputs.estop || inputs.limit_x || inputs.limit_y || inputs.limit_z) {
            state->aborted = true;
            state->active = false;
            hal_stepper_enable(false);
            return false;
        }
    }

    return !state->aborted;
}
