#include "drv_timebase.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint32_t g_millis = 0;

void sys_tick_handler(void) {
    g_millis++;
}

void drv_timebase_init(void) {
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    systick_set_reload((rcc_ahb_frequency / 8U / 1000U) - 1U);
    systick_interrupt_enable();
    systick_counter_enable();
}

uint32_t drv_timebase_millis(void) {
    return g_millis;
}

uint32_t drv_timebase_micros(void) {
    return g_millis * 1000U;
}

void drv_timebase_delay_ms(uint32_t ms) {
    const uint32_t deadline = drv_timebase_millis() + ms;
    while ((int32_t)(deadline - drv_timebase_millis()) > 0) {
    }
}
