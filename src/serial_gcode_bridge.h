#pragma once

#include <stddef.h>
#include <stdint.h>

#include "gcode.h"
#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gcode_state_t gcode;
    float steps_per_mm[HAL_AXIS_MAX];
    uint32_t step_pulse_delay_us;
} serial_gcode_bridge_t;

void serial_gcode_bridge_init(serial_gcode_bridge_t *bridge);

gcode_status_t serial_gcode_bridge_process_line(serial_gcode_bridge_t *bridge,
                                                const char *line,
                                                char *response,
                                                size_t response_len);

#ifdef __cplusplus
}
#endif
