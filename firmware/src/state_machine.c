#include "state_machine.h"

void state_machine_init(state_machine_t *sm) {
    system_init(sm);
}

void state_machine_poll(state_machine_t *sm) {
    system_poll(sm);
}

void state_machine_process_line(state_machine_t *sm, const char *line) {
    system_process_line(sm, line);
}

void state_machine_feed_hold(state_machine_t *sm) {
    system_feed_hold(sm);
}

void state_machine_cycle_start(state_machine_t *sm) {
    system_cycle_start(sm);
}

void state_machine_soft_reset(state_machine_t *sm) {
    system_soft_reset(sm);
}
