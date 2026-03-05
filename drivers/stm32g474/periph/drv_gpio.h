#pragma once

#include <stdbool.h>
#include <stdint.h>

void drv_gpio_init_safe(void);
void drv_gpio_stepper_enable(bool enable);
void drv_gpio_set_dir(uint8_t axis, bool dir_positive);
void drv_gpio_emit_step_mask(uint32_t axis_mask);
void drv_gpio_clear_step_mask(uint32_t axis_mask);
void drv_gpio_clear_pending_steps(void);
uint32_t drv_gpio_read_inputs_raw(void);
