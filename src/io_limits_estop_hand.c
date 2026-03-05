#include "io_limits_estop_hand.h"

#include <string.h>

void io_limits_estop_hand_init(io_limits_estop_hand_t *io, uint32_t debounce_ms) {
    if (!io) return;
    memset(io, 0, sizeof(*io));
    io->debounce_ms = debounce_ms;
}

static void update_debounced(bool *stable, bool raw, uint32_t now_ms, uint32_t debounce_ms, uint32_t *last_change_ms) {
    if (*stable == raw) {
        *last_change_ms = now_ms;
        return;
    }

    if ((now_ms - *last_change_ms) >= debounce_ms) {
        *stable = raw;
        *last_change_ms = now_ms;
    }
}

void io_limits_estop_hand_poll(io_limits_estop_hand_t *io, uint32_t now_ms) {
    if (!io) return;

    hal_read_inputs(&io->raw);

    update_debounced(&io->stable.limit_x, io->raw.limit_x, now_ms, io->debounce_ms, &io->last_x_change_ms);
    update_debounced(&io->stable.limit_y, io->raw.limit_y, now_ms, io->debounce_ms, &io->last_y_change_ms);
    update_debounced(&io->stable.limit_z, io->raw.limit_z, now_ms, io->debounce_ms, &io->last_z_change_ms);
    update_debounced(&io->stable.estop, io->raw.estop, now_ms, io->debounce_ms, &io->last_estop_change_ms);

    io->limit_alarm = io->stable.limit_x || io->stable.limit_y || io->stable.limit_z;
    if (io->stable.estop) {
        io->estop_latched = true;
    }
}

void io_limits_estop_hand_clear_estop(io_limits_estop_hand_t *io) {
    if (!io) return;
    if (!io->stable.estop) {
        io->estop_latched = false;
    }
}

bool io_limits_estop_hand_estop_active(const io_limits_estop_hand_t *io) {
    return io && io->estop_latched;
}

bool io_limits_estop_hand_limit_alarm_active(const io_limits_estop_hand_t *io) {
    return io && io->limit_alarm;
}
