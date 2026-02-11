#include "terminal_cli.h"

#include "kin_corexy.h"

#include <stdio.h>

static bool write_joint_response(const kin_joint_t *joint, char *response, size_t response_len) {
    if (!joint || !response || response_len == 0) return false;

    int written = snprintf(response, response_len, "OK");
    if (written < 0 || (size_t)written >= response_len) return false;

    for (uint8_t i = 0; i < g_kin.joint_axes; i++) {
        int step = snprintf(response + written,
                            response_len - (size_t)written,
                            " J%u=%.3f",
                            (unsigned int)i,
                            (double)joint->v[i]);
        if (step < 0 || (size_t)step >= response_len - (size_t)written) return false;
        written += step;
    }

    return true;
}

void terminal_cli_init(terminal_cli_t *cli) {
    if (!cli) return;
    gcode_init(&cli->gcode);
    kin_corexy_install(NULL);
}

bool terminal_cli_cart_to_joint(const gcode_state_t *gcode, kin_joint_t *out_joint) {
    if (!gcode || !out_joint || !g_kin.cart_to_joint) return false;

    kin_cart_t cart = {{ 0.0f }};
    cart.v[0] = gcode->position_x;
    cart.v[1] = gcode->position_y;
    return g_kin.cart_to_joint(&cart, out_joint);
}

gcode_status_t terminal_cli_process_line(terminal_cli_t *cli,
                                         const char *line,
                                         char *response,
                                         size_t response_len) {
    if (!cli || !line || !response || response_len == 0) return GCODE_ERR_INVALID_PARAM;

    gcode_status_t status = gcode_process_line(&cli->gcode, line);
    if (status != GCODE_OK) {
        snprintf(response, response_len, "error: %s", gcode_status_string(status));
        return status;
    }

    kin_joint_t joint;
    if (!terminal_cli_cart_to_joint(&cli->gcode, &joint)) {
        snprintf(response, response_len, "error: kinematics unavailable");
        return GCODE_ERR_INVALID_TARGET;
    }

    if (!write_joint_response(&joint, response, response_len)) {
        snprintf(response, response_len, "error: response too long");
        return GCODE_ERR_OVERFLOW;
    }

    return status;
}
