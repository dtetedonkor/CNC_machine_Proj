#include "serial_gcode_bridge.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cnc_hal.h"

static const char FW_IDENTITY[] = "[FW:STM32G4 CNC_machine_Proj]";

typedef enum {
    SETTING_U32 = 0,
    SETTING_BOOL,
    SETTING_FLOAT,
} setting_type_t;

static bool parse_u32_str(const char *s, uint32_t *out) {
    if (!s || !*s || !out) {
        return false;
    }
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v > 0xFFFFFFFFul) {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static bool parse_float_str(const char *s, float *out) {
    if (!s || !*s || !out) {
        return false;
    }
    char *end = NULL;
    errno = 0;
    const float v = strtof(s, &end);
    if (errno != 0 || end == s || *end != '\0' || !isfinite(v)) {
        return false;
    }
    *out = v;
    return true;
}

static bool parse_bool01_str(const char *s, bool *out) {
    uint32_t v = 0u;
    if (!parse_u32_str(s, &v) || v > 1u || !out) {
        return false;
    }
    *out = (v == 1u);
    return true;
}

static bool setting_type_for_id(uint32_t id, setting_type_t *type) {
    if (!type) {
        return false;
    }
    switch (id) {
        case 4u:
        case 5u:
        case 6u:
        case 13u:
        case 20u:
        case 21u:
        case 22u:
            *type = SETTING_BOOL;
            return true;
        case 11u:
        case 12u:
        case 24u:
        case 25u:
        case 27u:
        case 100u:
        case 101u:
        case 102u:
        case 110u:
        case 111u:
        case 112u:
        case 120u:
        case 121u:
        case 122u:
        case 130u:
        case 131u:
        case 132u:
            *type = SETTING_FLOAT;
            return true;
        case 0u:
        case 1u:
        case 2u:
        case 3u:
        case 10u:
        case 23u:
        case 26u:
        case 30u:
        case 31u:
            *type = SETTING_U32;
            return true;
        default:
            return false;
    }
}

static bool get_setting_value(const serial_gcode_bridge_t *bridge, uint32_t id, float *out_f, uint32_t *out_u, bool *out_b) {
    if (!bridge) {
        return false;
    }
    switch (id) {
        case 0u: *out_u = bridge->settings.step_pulse_time_us; return true;
        case 1u: *out_u = bridge->settings.step_idle_delay_ms; return true;
        case 2u: *out_u = bridge->settings.step_pulse_invert_mask; return true;
        case 3u: *out_u = bridge->settings.direction_invert_mask; return true;
        case 4u: *out_b = bridge->settings.step_enable_invert; return true;
        case 5u: *out_b = bridge->settings.limit_pins_invert; return true;
        case 6u: *out_b = bridge->settings.probe_pin_invert; return true;
        case 10u: *out_u = bridge->settings.status_report_mask; return true;
        case 11u: *out_f = bridge->settings.junction_deviation_mm; return true;
        case 12u: *out_f = bridge->settings.arc_tolerance_mm; return true;
        case 13u: *out_b = bridge->settings.report_inches; return true;
        case 20u: *out_b = bridge->settings.soft_limits_enable; return true;
        case 21u: *out_b = bridge->settings.hard_limits_enable; return true;
        case 22u: *out_b = bridge->settings.homing_cycle_enable; return true;
        case 23u: *out_u = bridge->settings.homing_direction_invert_mask; return true;
        case 24u: *out_f = bridge->settings.homing_locate_feed_mm_per_min; return true;
        case 25u: *out_f = bridge->settings.homing_seek_mm_per_min; return true;
        case 26u: *out_u = bridge->settings.homing_debounce_ms; return true;
        case 27u: *out_f = bridge->settings.homing_pull_off_mm; return true;
        case 30u: *out_u = bridge->settings.spindle_max_rpm; return true;
        case 31u: *out_u = bridge->settings.spindle_min_rpm; return true;
        case 100u: *out_f = bridge->steps_per_mm[HAL_AXIS_X]; return true;
        case 101u: *out_f = bridge->steps_per_mm[HAL_AXIS_Y]; return true;
        case 102u: *out_f = bridge->steps_per_mm[HAL_AXIS_Z]; return true;
        case 110u: *out_f = bridge->settings.max_rate_mm_per_min[0]; return true;
        case 111u: *out_f = bridge->settings.max_rate_mm_per_min[1]; return true;
        case 112u: *out_f = bridge->settings.max_rate_mm_per_min[2]; return true;
        case 120u: *out_f = bridge->settings.accel_mm_per_s2[0]; return true;
        case 121u: *out_f = bridge->settings.accel_mm_per_s2[1]; return true;
        case 122u: *out_f = bridge->settings.accel_mm_per_s2[2]; return true;
        case 130u: *out_f = bridge->settings.max_travel_mm[0]; return true;
        case 131u: *out_f = bridge->settings.max_travel_mm[1]; return true;
        case 132u: *out_f = bridge->settings.max_travel_mm[2]; return true;
        default: return false;
    }
}

static bool set_setting_value(serial_gcode_bridge_t *bridge, uint32_t id, const char *value) {
    setting_type_t type = SETTING_U32;
    if (!bridge || !setting_type_for_id(id, &type)) {
        return false;
    }

    uint32_t u32 = 0u;
    float f = 0.0f;
    bool b = false;
    if ((type == SETTING_U32 && !parse_u32_str(value, &u32)) ||
        (type == SETTING_FLOAT && !parse_float_str(value, &f)) ||
        (type == SETTING_BOOL && !parse_bool01_str(value, &b))) {
        return false;
    }

    switch (id) {
        case 0u: bridge->settings.step_pulse_time_us = u32; bridge->step_pulse_delay_us = u32; return true;
        case 1u: bridge->settings.step_idle_delay_ms = u32; return true;
        case 2u: bridge->settings.step_pulse_invert_mask = u32; return true;
        case 3u: bridge->settings.direction_invert_mask = u32; return true;
        case 4u: bridge->settings.step_enable_invert = b; return true;
        case 5u: bridge->settings.limit_pins_invert = b; return true;
        case 6u: bridge->settings.probe_pin_invert = b; return true;
        case 10u: bridge->settings.status_report_mask = u32; return true;
        case 11u: bridge->settings.junction_deviation_mm = f; return true;
        case 12u: bridge->settings.arc_tolerance_mm = f; return true;
        case 13u: bridge->settings.report_inches = b; return true;
        case 20u: bridge->settings.soft_limits_enable = b; return true;
        case 21u: bridge->settings.hard_limits_enable = b; return true;
        case 22u: bridge->settings.homing_cycle_enable = b; return true;
        case 23u: bridge->settings.homing_direction_invert_mask = u32; return true;
        case 24u: bridge->settings.homing_locate_feed_mm_per_min = f; return true;
        case 25u: bridge->settings.homing_seek_mm_per_min = f; return true;
        case 26u: bridge->settings.homing_debounce_ms = u32; return true;
        case 27u: bridge->settings.homing_pull_off_mm = f; return true;
        case 30u: bridge->settings.spindle_max_rpm = u32; return true;
        case 31u: bridge->settings.spindle_min_rpm = u32; return true;
        case 100u: bridge->steps_per_mm[HAL_AXIS_X] = f; return true;
        case 101u: bridge->steps_per_mm[HAL_AXIS_Y] = f; return true;
        case 102u: bridge->steps_per_mm[HAL_AXIS_Z] = f; return true;
        case 110u: bridge->settings.max_rate_mm_per_min[0] = f; return true;
        case 111u: bridge->settings.max_rate_mm_per_min[1] = f; return true;
        case 112u: bridge->settings.max_rate_mm_per_min[2] = f; return true;
        case 120u: bridge->settings.accel_mm_per_s2[0] = f; return true;
        case 121u: bridge->settings.accel_mm_per_s2[1] = f; return true;
        case 122u: bridge->settings.accel_mm_per_s2[2] = f; return true;
        case 130u: bridge->settings.max_travel_mm[0] = f; return true;
        case 131u: bridge->settings.max_travel_mm[1] = f; return true;
        case 132u: bridge->settings.max_travel_mm[2] = f; return true;
        default: return false;
    }
}

static size_t append_setting_line(char *dst, size_t cap, size_t off, uint32_t id, const serial_gcode_bridge_t *bridge) {
    setting_type_t type = SETTING_U32;
    if (!dst || cap == 0u || !bridge || !setting_type_for_id(id, &type)) {
        return off;
    }

    float f = 0.0f;
    uint32_t u = 0u;
    bool b = false;
    if (!get_setting_value(bridge, id, &f, &u, &b)) {
        return off;
    }

    int n = 0;
    if (type == SETTING_FLOAT) {
        n = snprintf(dst + off, (off < cap) ? (cap - off) : 0u, "$%lu=%.3f\n", (unsigned long)id, (double)f);
    } else if (type == SETTING_BOOL) {
        n = snprintf(dst + off, (off < cap) ? (cap - off) : 0u, "$%lu=%u\n", (unsigned long)id, b ? 1u : 0u);
    } else {
        n = snprintf(dst + off, (off < cap) ? (cap - off) : 0u, "$%lu=%lu\n", (unsigned long)id, (unsigned long)u);
    }
    if (n <= 0) {
        return off;
    }
    size_t inc = (size_t)n;
    if (off + inc >= cap) {
        return cap - 1u;
    }
    return off + inc;
}

static void write_settings_dump(const serial_gcode_bridge_t *bridge, char *response, size_t response_len) {
    static const uint32_t setting_order[] = {
        0u, 1u, 2u, 3u, 4u, 5u, 6u,
        10u, 11u, 12u, 13u,
        20u, 21u, 22u, 23u, 24u, 25u, 26u, 27u,
        30u, 31u,
        100u, 101u, 102u,
        110u, 111u, 112u,
        120u, 121u, 122u,
        130u, 131u, 132u
    };
    size_t off = 0u;
    if (!response || response_len == 0u) {
        return;
    }
    response[0] = '\0';
    for (size_t i = 0u; i < (sizeof(setting_order) / sizeof(setting_order[0])); i++) {
        off = append_setting_line(response, response_len, off, setting_order[i], bridge);
        if (off >= (response_len - 1u)) {
            break;
        }
    }
}

static bool parse_setting_assignment(const char *line, uint32_t *id_out, const char **value_out) {
    if (!line || !id_out || !value_out || line[0] != '$') {
        return false;
    }
    const char *eq = strchr(line + 1, '=');
    if (!eq || eq == (line + 1) || eq[1] == '\0') {
        return false;
    }
    const size_t id_len = (size_t)(eq - (line + 1));
    if (id_len >= 8u) {
        return false;
    }
    char id_buf[8];
    memcpy(id_buf, line + 1, id_len);
    id_buf[id_len] = '\0';

    uint32_t id = 0u;
    if (!parse_u32_str(id_buf, &id)) {
        return false;
    }
    *id_out = id;
    *value_out = eq + 1;
    return true;
}

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

static void clear_axes_by_mask(uint32_t axis_mask) {
    for (hal_axis_t axis = HAL_AXIS_X; axis < HAL_AXIS_MAX; axis++) {
        if ((axis_mask & (1u << axis)) != 0u) {
            hal_stepper_step_clear(axis);
        }
    }
}

static bool drive_xy_motion(serial_gcode_bridge_t *bridge,
                            float start_x,
                            float start_y,
                            float end_x,
                            float end_y) {
    const float dx = end_x - start_x;
    const float dy = end_y - start_y;

    const uint32_t x_steps = (uint32_t)lroundf(fabsf(dx) * bridge->steps_per_mm[HAL_AXIS_X]);
    const uint32_t y_steps = (uint32_t)lroundf(fabsf(dy) * bridge->steps_per_mm[HAL_AXIS_Y]);

    if (x_steps == 0u && y_steps == 0u) {
        return true;
    }

    hal_stepper_enable(true);
    hal_stepper_set_dir(HAL_AXIS_X, dx >= 0.0f);
    hal_stepper_set_dir(HAL_AXIS_Y, dy >= 0.0f);

    if (x_steps >= y_steps) {
        uint32_t err = x_steps / 2u;
        for (uint32_t i = 0; i < x_steps; i++) {
            uint32_t pulse_mask = (1u << HAL_AXIS_X);

            err += y_steps;
            const bool pulse_y = (err >= x_steps && y_steps > 0u);
            if (pulse_y) {
                pulse_mask |= (1u << HAL_AXIS_Y);
                err -= x_steps;
            }

            hal_stepper_pulse_mask(pulse_mask);
            wait_us(bridge->step_pulse_delay_us);
            clear_axes_by_mask(pulse_mask);

            hal_poll();
            hal_inputs_t inputs;
            hal_read_inputs(&inputs);
            if (inputs.estop || inputs.limit_x || inputs.limit_y || inputs.limit_z) {
                hal_stepper_enable(false);
                return false;
            }
        }
    } else {
        uint32_t err = y_steps / 2u;
        for (uint32_t i = 0; i < y_steps; i++) {
            uint32_t pulse_mask = (1u << HAL_AXIS_Y);

            err += x_steps;
            const bool pulse_x = (err >= y_steps && x_steps > 0u);
            if (pulse_x) {
                pulse_mask |= (1u << HAL_AXIS_X);
                err -= y_steps;
            }

            hal_stepper_pulse_mask(pulse_mask);
            wait_us(bridge->step_pulse_delay_us);
            clear_axes_by_mask(pulse_mask);

            hal_poll();
            hal_inputs_t inputs;
            hal_read_inputs(&inputs);
            if (inputs.estop || inputs.limit_x || inputs.limit_y || inputs.limit_z) {
                hal_stepper_enable(false);
                return false;
            }
        }
    }

    return true;
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
    bridge->settings.step_pulse_time_us = 50u;
    bridge->settings.step_idle_delay_ms = 25u;
    bridge->settings.step_pulse_invert_mask = 0u;
    bridge->settings.direction_invert_mask = 0u;
    bridge->settings.step_enable_invert = false;
    bridge->settings.limit_pins_invert = false;
    bridge->settings.probe_pin_invert = false;
    bridge->settings.status_report_mask = 3u;
    bridge->settings.junction_deviation_mm = 0.010f;
    bridge->settings.arc_tolerance_mm = 0.002f;
    bridge->settings.report_inches = false;
    bridge->settings.soft_limits_enable = false;
    bridge->settings.hard_limits_enable = false;
    bridge->settings.homing_cycle_enable = false;
    bridge->settings.homing_direction_invert_mask = 0u;
    bridge->settings.homing_locate_feed_mm_per_min = 100.0f;
    bridge->settings.homing_seek_mm_per_min = 500.0f;
    bridge->settings.homing_debounce_ms = 250u;
    bridge->settings.homing_pull_off_mm = 1.0f;
    bridge->settings.spindle_max_rpm = 10000u;
    bridge->settings.spindle_min_rpm = 0u;
    bridge->settings.max_rate_mm_per_min[0] = 3000.0f;
    bridge->settings.max_rate_mm_per_min[1] = 3000.0f;
    bridge->settings.max_rate_mm_per_min[2] = 500.0f;
    bridge->settings.accel_mm_per_s2[0] = 200.0f;
    bridge->settings.accel_mm_per_s2[1] = 200.0f;
    bridge->settings.accel_mm_per_s2[2] = 50.0f;
    bridge->settings.max_travel_mm[0] = 200.0f;
    bridge->settings.max_travel_mm[1] = 200.0f;
    bridge->settings.max_travel_mm[2] = 50.0f;
}

void serial_gcode_bridge_set_motion_backend(serial_gcode_bridge_t *bridge,
                                            bool (*backend)(void *ctx,
                                                            float start_x,
                                                            float start_y,
                                                            float end_x,
                                                            float end_y,
                                                            const float *steps_per_mm,
                                                            uint32_t step_pulse_delay_us),
                                            void *backend_ctx) {
    if (!bridge) {
        return;
    }

    bridge->motion_backend = backend;
    bridge->motion_backend_ctx = backend_ctx;
}

gcode_status_t serial_gcode_bridge_process_line(serial_gcode_bridge_t *bridge,
                                                const char *line,
                                                char *response,
                                                size_t response_len) {
    if (!bridge || !line || !response || response_len == 0u) {
        return GCODE_ERR_INVALID_PARAM;
    }

    if (line_is_simple_cmd(line, "$I")) {
        snprintf(response, response_len, "%s", FW_IDENTITY);
        return GCODE_OK;
    }

    if (line_is_simple_cmd(line, "$$")) {
        write_settings_dump(bridge, response, response_len);
        return GCODE_OK;
    }

    uint32_t setting_id = 0u;
    const char *setting_value = NULL;
    if (parse_setting_assignment(line, &setting_id, &setting_value)) {
        if (setting_id == 32u) {
            snprintf(response, response_len, "error: unsupported setting $32");
            return GCODE_ERR_UNSUPPORTED_CMD;
        }
        setting_type_t parsed_type = SETTING_U32;
        if (!setting_type_for_id(setting_id, &parsed_type)) {
            snprintf(response, response_len, "error: unknown setting $%lu", (unsigned long)setting_id);
            return GCODE_ERR_INVALID_PARAM;
        }
        if (!set_setting_value(bridge, setting_id, setting_value)) {
            snprintf(response,
                     response_len,
                     "error: invalid value for setting $%lu",
                     (unsigned long)setting_id);
            return GCODE_ERR_INVALID_PARAM;
        }
        snprintf(response, response_len, "OK");
        return GCODE_OK;
    }

    /* Status query: report current position */
    if (line_is_simple_cmd(line, "?") || line_is_simple_cmd(line, "$")) {
        float x = 0.0f;
        float y = 0.0f;
        gcode_get_position(&bridge->gcode, &x, &y);

        /* Report position in thousandths of mm to avoid printf float support. */
        const int32_t x_milli = (int32_t)lroundf(x * 1000.0f);
        const int32_t y_milli = (int32_t)lroundf(y * 1000.0f);

        snprintf(response, response_len, "X:%ld Y:%ld (x0.001mm)",
                 (long)x_milli, (long)y_milli);
        return GCODE_OK;
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

    bool motion_ok = false;
    if (bridge->motion_backend != NULL) {
        motion_ok = bridge->motion_backend(bridge->motion_backend_ctx,
                                           start_x,
                                           start_y,
                                           end_x,
                                           end_y,
                                           bridge->steps_per_mm,
                                           bridge->step_pulse_delay_us);
    } else {
        motion_ok = drive_xy_motion(bridge, start_x, start_y, end_x, end_y);
    }

    if (!motion_ok) {
        snprintf(response, response_len, "error: safety input active");
        return GCODE_ERR_INVALID_TARGET;
    }

    snprintf(response, response_len, "OK");
    return GCODE_OK;
}
