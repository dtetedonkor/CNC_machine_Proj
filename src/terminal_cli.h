#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "gcode.h"
#include "kinematics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gcode_state_t gcode;
} terminal_cli_t;

void terminal_cli_init(terminal_cli_t *cli);
gcode_status_t terminal_cli_process_line(terminal_cli_t *cli,
                                         const char *line,
                                         char *response,
                                         size_t response_len);
bool terminal_cli_cart_to_joint(const gcode_state_t *gcode, kin_joint_t *out_joint);

#ifdef __cplusplus
}
#endif
