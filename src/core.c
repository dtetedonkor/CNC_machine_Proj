#include "core.h"

#include <stdio.h>
#include <string.h>

static void copy_response(char *response, size_t response_len, const char *text) {
    if (!response || response_len == 0u) {
        return;
    }

    if (!text) {
        response[0] = '\0';
        return;
    }

    snprintf(response, response_len, "%s", text);
}

static void core_refresh_inputs(core_t *core) {
    if (!core) {
        return;
    }

    hal_inputs_t sampled = {0};
    hal_read_inputs(&sampled);
    core_limits_update(core, &sampled);
}

static gcode_status_t gcode_execute_or_queue(core_t *core,
                                             const char *line,
                                             char *response,
                                             size_t response_len) {
    return serial_gcode_bridge_process_line(&core->bridge, line, response, response_len);
}

static gcode_status_t protocol_process_line(core_t *core,
                                            const char *line,
                                            char *response,
                                            size_t response_len) {
    char normalized[PROTOCOL_LINE_MAX + 1];
    proto_line_status_t proto_st = PROTO_LINE_EMPTY;
    gcode_status_t gcode_st = GCODE_ERR_INVALID_PARAM;

    protocol_feed_bytes(&core->protocol, (const uint8_t *)line, strlen(line));
    protocol_feed_bytes(&core->protocol, (const uint8_t *)"\n", 1u);

    if (!protocol_pop_line(&core->protocol, normalized, sizeof(normalized), &proto_st)) {
        copy_response(response, response_len, "OK");
        return GCODE_OK;
    }

    if (proto_st == PROTO_LINE_OVERFLOW) {
        copy_response(response, response_len, "error: line overflow");
        return GCODE_ERR_OVERFLOW;
    }

    if (proto_st == PROTO_LINE_BAD_CHAR) {
        copy_response(response, response_len, "error: bad character");
        return GCODE_ERR_INVALID_PARAM;
    }

    gcode_st = gcode_execute_or_queue(core, normalized, response, response_len);
    return gcode_st;
}

static gcode_status_t serial_gcode_bridge_submit(core_t *core,
                                                 const char *line,
                                                 char *response,
                                                 size_t response_len) {
    return protocol_process_line(core, line, response, response_len);
}

static gcode_status_t uart_rx_line_complete(core_t *core,
                                            const char *line,
                                            char *response,
                                            size_t response_len) {
    return serial_gcode_bridge_submit(core, line, response, response_len);
}

void core_init(core_t *core) {
    if (!core) {
        return;
    }

    memset(core, 0, sizeof(*core));
    serial_gcode_bridge_init(&core->bridge);

    const proto_config_t cfg = {
        .strip_semicolon_comments = true,
        .strip_paren_comments = true,
        .allow_dollar_commands = true,
        .to_uppercase = true,
    };
    protocol_init(&core->protocol, &cfg, NULL, NULL, NULL);
}

gcode_status_t core_submit_line(core_t *core, const char *line, char *response, size_t response_len) {
    if (!core || !line || !response || response_len == 0u) {
        return GCODE_ERR_INVALID_PARAM;
    }

    core_refresh_inputs(core);
    if (core->estop_latched || core->limit_alarm) {
        hal_stepper_enable(false);
        copy_response(response, response_len, "error: safety input active");
        return GCODE_ERR_INVALID_TARGET;
    }

    return uart_rx_line_complete(core, line, response, response_len);
}

void core_poll(core_t *core) {
    if (!core) {
        return;
    }

    hal_poll();
    core_refresh_inputs(core);
}

size_t core_get_status(core_t *core, char *out, size_t out_len) {
    if (!core || !out || out_len == 0u) {
        return 0u;
    }

    core_refresh_inputs(core);
    return (size_t)snprintf(out,
                            out_len,
                            "<%s|LIM:%u|ESTOP:%u>",
                            core->limit_alarm ? "ALARM" : "IDLE",
                            core->limit_alarm ? 1u : 0u,
                            core->estop_latched ? 1u : 0u);
}

void core_estop(core_t *core) {
    if (!core) {
        return;
    }

    core->estop_latched = true;
    hal_stepper_enable(false);
}

void core_limits_update(core_t *core, const hal_inputs_t *limits) {
    if (!core || !limits) {
        return;
    }

    core->limits = *limits;
    core->limit_alarm = limits->limit_x || limits->limit_y || limits->limit_z;
    if (limits->estop) {
        core->estop_latched = true;
    }
}
