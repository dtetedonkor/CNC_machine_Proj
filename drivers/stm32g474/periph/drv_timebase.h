#pragma once

#include <stdint.h>

void drv_timebase_init(void);
uint32_t drv_timebase_millis(void);
uint32_t drv_timebase_micros(void);
void drv_timebase_delay_ms(uint32_t ms);
