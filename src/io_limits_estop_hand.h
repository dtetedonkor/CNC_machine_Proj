#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cnc_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t debounce_ms;

    hal_inputs_t raw;
    hal_inputs_t stable;

    bool estop_latched;
    bool limit_alarm;

    uint32_t last_x_change_ms;
    uint32_t last_y_change_ms;
    uint32_t last_z_change_ms;
    uint32_t last_estop_change_ms;
} io_limits_estop_hand_t;

void io_limits_estop_hand_init(io_limits_estop_hand_t *io, uint32_t debounce_ms);
void io_limits_estop_hand_poll(io_limits_estop_hand_t *io, uint32_t now_ms);
void io_limits_estop_hand_clear_estop(io_limits_estop_hand_t *io);

bool io_limits_estop_hand_estop_active(const io_limits_estop_hand_t *io);
bool io_limits_estop_hand_limit_alarm_active(const io_limits_estop_hand_t *io);

#ifdef __cplusplus
}
#endif
