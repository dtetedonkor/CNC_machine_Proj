#pragma once

#include <stdint.h>

typedef uint32_t (*drv_stepper_tick_cb_t)(void);

void drv_stepper_timer_init(uint32_t tick_hz, drv_stepper_tick_cb_t step_cb);
void drv_stepper_timer_start(void);
void drv_stepper_timer_stop(void);
