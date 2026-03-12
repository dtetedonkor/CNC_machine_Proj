#include "board_stm32g474.h"

#include "main.h"

#ifndef BOARD_STM32G474_STEP_PULSE_US
#define BOARD_STM32G474_STEP_PULSE_US 2u
#endif

extern UART_HandleTypeDef hlpuart1;

hal_status_t board_stm32g474_init(void) {
    return HAL_OK;
}

void board_stm32g474_start(void) {}

void board_stm32g474_deinit(void) {}

uint32_t board_stm32g474_millis(void) {
    return HAL_GetTick();
}

uint32_t board_stm32g474_micros(void) {
    return HAL_GetTick() * 1000u;
}

void board_stm32g474_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

size_t board_stm32g474_uart_write(const uint8_t *src, size_t len) {
    if (!src || len == 0u) {
        return 0u;
    }

    if (HAL_UART_Transmit(&hlpuart1, (uint8_t *)src, (uint16_t)len, 1000u) != HAL_OK) {
        return 0u;
    }
    return len;
}

size_t board_stm32g474_uart_read(uint8_t *dst, size_t cap) {
    (void)dst;
    (void)cap;
    return 0u;
}

void board_stm32g474_stepper_enable(bool en) {
    (void)en;
}

void board_stm32g474_stepper_set_dir(hal_axis_t axis, bool dir_positive) {
    (void)axis;
    (void)dir_positive;
}

void board_stm32g474_stepper_pulse_mask(uint32_t axis_mask) {
    (void)axis_mask;
    for (volatile uint32_t i = 0; i < (BOARD_STM32G474_STEP_PULSE_US * 16u); ++i) {
    }
}

void board_stm32g474_stepper_step_clear(hal_axis_t axis) {
    (void)axis;
}

void board_stm32g474_read_inputs(hal_inputs_t *out) {
    if (!out) {
        return;
    }
    out->limit_x = false;
    out->limit_y = false;
    out->limit_z = false;
    out->estop = false;
    out->probe = false;
}

void board_stm32g474_poll(void) {}

void board_stm32g474_tick_1khz_isr(void) {}
