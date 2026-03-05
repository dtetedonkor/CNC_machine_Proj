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

static void emit_xy_motion(serial_gcode_bridge_t *bridge,
                           float start_x,
                           float start_y,
                           float end_x,
                           float end_y) {
    const float dx = end_x - start_x;
    const float dy = end_y - start_y;

    const uint32_t x_steps = (uint32_t)lroundf(fabsf(dx) * bridge->steps_per_mm_x);
    const uint32_t y_steps = (uint32_t)lroundf(fabsf(dy) * bridge->steps_per_mm_y);

    const uint32_t max_steps = (x_steps > y_steps) ? x_steps : y_steps;
    if (max_steps == 0u) {
        return;
    }

    hal_stepper_enable(true);
    hal_stepper_set_dir(HAL_AXIS_X, dx >= 0.0f);
    hal_stepper_set_dir(HAL_AXIS_Y, dy >= 0.0f);

    for (uint32_t i = 0; i < max_steps; i++) {
        if (i < x_steps) {
            hal_stepper_step_pulse(HAL_AXIS_X);
        }
        if (i < y_steps) {
            hal_stepper_step_pulse(HAL_AXIS_Y);
        }

        hal_delay_ms(bridge->step_pulse_delay_ms);

        if (i < x_steps) {
            hal_stepper_step_clear(HAL_AXIS_X);
        }
        if (i < y_steps) {
            hal_stepper_step_clear(HAL_AXIS_Y);
        }
    }
}

void serial_gcode_bridge_init(serial_gcode_bridge_t *bridge) {
    if (!bridge) {
        return;
    }

    memset(bridge, 0, sizeof(*bridge));
    gcode_init(&bridge->gcode);
    bridge->steps_per_mm_x = 80.0f;
    bridge->steps_per_mm_y = 80.0f;
    bridge->step_pulse_delay_ms = 1u;
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
    emit_xy_motion(bridge, start_x, start_y, end_x, end_y);

    snprintf(response, response_len, "OK");
    return GCODE_OK;
}
