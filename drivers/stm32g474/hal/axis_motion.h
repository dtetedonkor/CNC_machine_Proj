#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile bool active;
    volatile bool aborted;
    volatile uint32_t remaining_primary_steps;
    uint32_t primary_steps_total;
    uint32_t minor_steps_total;
    uint32_t error_accum;
    uint32_t primary_axis_mask;
    uint32_t minor_axis_mask;
    uint32_t tick_divider;
    uint32_t tick_counter;
} axis_motion_state_t;

void axis_motion_init(axis_motion_state_t *state);
bool axis_motion_start_xy(axis_motion_state_t *state,
                          int32_t x_steps,
                          int32_t y_steps,
                          bool x_dir_positive,
                          bool y_dir_positive,
                          uint32_t step_period_us);
uint32_t axis_motion_step_tick(axis_motion_state_t *state);
bool axis_motion_wait_complete(axis_motion_state_t *state);
