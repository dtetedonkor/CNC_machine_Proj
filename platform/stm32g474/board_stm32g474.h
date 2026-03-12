#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../src/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

hal_status_t board_stm32g474_init(void);
void board_stm32g474_start(void);
void board_stm32g474_deinit(void);

uint32_t board_stm32g474_millis(void);
uint32_t board_stm32g474_micros(void);
void board_stm32g474_delay_ms(uint32_t ms);

size_t board_stm32g474_uart_write(const uint8_t *src, size_t len);
size_t board_stm32g474_uart_read(uint8_t *dst, size_t cap);

void board_stm32g474_stepper_enable(bool en);
void board_stm32g474_stepper_set_dir(hal_axis_t axis, bool dir_positive);
void board_stm32g474_stepper_pulse_mask(uint32_t axis_mask);
void board_stm32g474_stepper_step_clear(hal_axis_t axis);

void board_stm32g474_read_inputs(hal_inputs_t *out);
void board_stm32g474_poll(void);
void board_stm32g474_tick_1khz_isr(void);

#ifdef __cplusplus
}
#endif
