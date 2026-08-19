#include "cnc_hal.h"
#include "stm32g4xx_hal.h"

uint32_t hal_micros(void)
{
    return (uint32_t)(HAL_GetTick() * 1000u);
}

void hal_poll(void)
{
}

void hal_stepper_enable(bool en)
{
    (void)en;
}

void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive)
{
    (void)axis;
    (void)dir_positive;
}

void hal_stepper_pulse_mask(uint32_t axis_mask)
{
    (void)axis_mask;
}

void hal_stepper_step_clear(hal_axis_t axis)
{
    (void)axis;
}

void hal_read_inputs(hal_inputs_t *out)
{
    if (!out) return;
    out->limit_x = false;
    out->limit_y = false;
    out->limit_z = false;
    out->estop   = false;
    out->probe   = false;
}
