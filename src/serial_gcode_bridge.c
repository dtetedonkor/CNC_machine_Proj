#include "serial_gcode_bridge.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hal.h"

static bool line_is_simple_cmd(const char *line, const char *cmd) {
    if (!line || !cmd) {
        return false;
    }

    while (*line && isspace((unsigned char)*line)) {
        line++;
    }

    size_t idx = 0;
    while (cmd[idx] != '\0') {
        if (toupper((unsigned char)line[idx]) != cmd[idx]) {
            return false;
        }
        idx++;
    }

    while (line[idx] && isspace((unsigned char)line[idx])) {
        idx++;
    }

    return line[idx] == '\0';
}

static void wait_us(uint32_t delay_us) {
    const uint32_t start = hal_micros();
    while ((uint32_t)(hal_micros() - start) < delay_us) {
        hal_poll();
    }
}

static void drive_xy_motion(serial_gcode_bridge_t *bridge,
                            float start_x,
                            float start_y,
                            float end_x,
                            float end_y) {
    const float dx = end_x - start_x;
    const float dy = end_y - start_y;

    const uint32_t x_steps = (uint32_t)lroundf(fabsf(dx) * bridge->steps_per_mm[HAL_AXIS_X]);
    const uint32_t y_steps = (uint32_t)lroundf(fabsf(dy) * bridge->steps_per_mm[HAL_AXIS_Y]);

    if (x_steps == 0u && y_steps == 0u) {
        return;
    }

    hal_stepper_enable(true);
    hal_stepper_set_dir(HAL_AXIS_X, dx >= 0.0f);
    hal_stepper_set_dir(HAL_AXIS_Y, dy >= 0.0f);

    if (x_steps >= y_steps) {
        uint32_t err = x_steps / 2u;
        for (uint32_t i = 0; i < x_steps; i++) {
            hal_stepper_step_pulse(HAL_AXIS_X);

            err += y_steps;
            const bool pulse_y = (err >= x_steps && y_steps > 0u);
            if (pulse_y) {
                hal_stepper_step_pulse(HAL_AXIS_Y);
                err -= x_steps;
            }

            wait_us(bridge->step_pulse_delay_us);
            hal_stepper_step_clear(HAL_AXIS_X);
            if (pulse_y) {
                hal_stepper_step_clear(HAL_AXIS_Y);
            }

            hal_poll();
            hal_inputs_t inputs;
            hal_read_inputs(&inputs);
            if (inputs.estop || inputs.limit_x || inputs.limit_y || inputs.limit_z) {
                hal_stepper_enable(false);
                return;
            }
        }
    } else {
        uint32_t err = y_steps / 2u;
        for (uint32_t i = 0; i < y_steps; i++) {
            hal_stepper_step_pulse(HAL_AXIS_Y);

            err += x_steps;
            const bool pulse_x = (err >= y_steps && x_steps > 0u);
            if (pulse_x) {
                hal_stepper_step_pulse(HAL_AXIS_X);
                err -= y_steps;
            }

            wait_us(bridge->step_pulse_delay_us);
            hal_stepper_step_clear(HAL_AXIS_Y);
            if (pulse_x) {
                hal_stepper_step_clear(HAL_AXIS_X);
            }

            hal_poll();
            hal_inputs_t inputs;
            hal_read_inputs(&inputs);
            if (inputs.estop || inputs.limit_x || inputs.limit_y || inputs.limit_z) {
                hal_stepper_enable(false);
                return;
            }
        }
    }
}

void serial_gcode_bridge_init(serial_gcode_bridge_t *bridge) {
    if (!bridge) {
        return;
    }

    memset(bridge, 0, sizeof(*bridge));
    gcode_init(&bridge->gcode);
    for (uint8_t axis = 0; axis < HAL_AXIS_MAX; axis++) {
        bridge->steps_per_mm[axis] = 80.0f;
    }
    bridge->step_pulse_delay_us = 50u;
}

gcode_status_t serial_gcode_bridge_process_line(serial_gcode_bridge_t *bridge,
                                                const char *line,
                                                char *response,
                                                size_t response_len) {
    if (!bridge || !line || !response || response_len == 0u) {
        return GCODE_ERR_INVALID_PARAM;
    }

    if (line_is_simple_cmd(line, "M17")) {
        hal_stepper_enable(true);
        snprintf(response, response_len, "OK");
        return GCODE_OK;
    }

    if (line_is_simple_cmd(line, "M18")) {
        hal_stepper_enable(false);
        snprintf(response, response_len, "OK");
        return GCODE_OK;
    }

    float start_x = 0.0f;
    float start_y = 0.0f;
    gcode_get_position(&bridge->gcode, &start_x, &start_y);

    const gcode_status_t status = gcode_process_line(&bridge->gcode, line);
    if (status != GCODE_OK) {
        snprintf(response, response_len, "error: %s", gcode_status_string(status));
        return status;
    }

    float end_x = 0.0f;
    float end_y = 0.0f;
    gcode_get_position(&bridge->gcode, &end_x, &end_y);
    drive_xy_motion(bridge, start_x, start_y, end_x, end_y);

    snprintf(response, response_len, "OK");
    return GCODE_OK;
}
