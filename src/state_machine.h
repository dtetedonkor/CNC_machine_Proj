#pragma once

#include "system_state.h"

typedef system_context_t state_machine_t;

void state_machine_init(state_machine_t *sm);
void state_machine_poll(state_machine_t *sm);
void state_machine_process_line(state_machine_t *sm, const char *line);
void state_machine_feed_hold(state_machine_t *sm);
void state_machine_cycle_start(state_machine_t *sm);
void state_machine_soft_reset(state_machine_t *sm);
