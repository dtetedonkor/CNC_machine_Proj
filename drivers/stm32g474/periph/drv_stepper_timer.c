#include "drv_stepper_timer.h"

#include "../board/board_pins.h"
#include "drv_gpio.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

static drv_stepper_tick_cb_t g_step_cb = 0;

void tim6_dac_isr(void) {
    if (timer_get_flag(BOARD_STEP_TIMER, TIM_SR_UIF) == 0) {
        return;
    }

    timer_clear_flag(BOARD_STEP_TIMER, TIM_SR_UIF);

    drv_gpio_clear_pending_steps();

    if (g_step_cb != 0) {
        const uint32_t step_mask = g_step_cb();
        if (step_mask != 0) {
            drv_gpio_emit_step_mask(step_mask);
        }
    }
}

void drv_stepper_timer_init(uint32_t tick_hz, drv_stepper_tick_cb_t step_cb) {
    const uint32_t timer_clk_hz = rcc_apb1_frequency;
    const uint32_t prescaler = (timer_clk_hz / 1000000U) - 1U;
    const uint32_t period = (1000000U / tick_hz) - 1U;

    g_step_cb = step_cb;

    rcc_periph_clock_enable(BOARD_STEP_TIMER_RCC);
    timer_disable_counter(BOARD_STEP_TIMER);
    timer_set_prescaler(BOARD_STEP_TIMER, prescaler);
    timer_set_period(BOARD_STEP_TIMER, period);
    timer_set_counter(BOARD_STEP_TIMER, 0);
    timer_enable_irq(BOARD_STEP_TIMER, TIM_DIER_UIE);

    nvic_enable_irq(BOARD_STEP_TIMER_IRQ);
}

void drv_stepper_timer_start(void) {
    timer_enable_counter(BOARD_STEP_TIMER);
}

void drv_stepper_timer_stop(void) {
    timer_disable_counter(BOARD_STEP_TIMER);
}
