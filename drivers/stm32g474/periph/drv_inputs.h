#pragma once

#include <stdint.h>

typedef struct {
    uint32_t state;
    uint32_t rising_edges;
} drv_inputs_snapshot_t;

void drv_inputs_init(void);
void drv_inputs_poll(void);
drv_inputs_snapshot_t drv_inputs_get(void);
