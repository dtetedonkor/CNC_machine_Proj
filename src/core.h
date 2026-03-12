#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "gcode.h"
#include "hal.h"
#include "protocol.h"
#include "serial_gcode_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    serial_gcode_bridge_t bridge;
    protocol_t protocol;
    hal_inputs_t limits;
    bool estop_latched;
    bool limit_alarm;
} core_t;

void core_init(core_t *core);
gcode_status_t core_submit_line(core_t *core, const char *line, char *response, size_t response_len);
void core_poll(core_t *core);
size_t core_get_status(core_t *core, char *out, size_t out_len);
void core_estop(core_t *core);
void core_limits_update(core_t *core, const hal_inputs_t *limits);

#ifdef __cplusplus
}
#endif
