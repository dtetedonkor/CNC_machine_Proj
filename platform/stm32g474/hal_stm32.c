#include "hal_stm32.h"

#include <string.h>

#include "board_stm32g474.h"

hal_status_t hal_stm32_init(void) {
    return board_stm32g474_init();
}

hal_status_t hal_init(void) {
    return hal_stm32_init();
}

void hal_start(void) {
    board_stm32g474_start();
}

void hal_deinit(void) {
    board_stm32g474_deinit();
}

uint32_t hal_millis(void) {
    return board_stm32g474_millis();
}

uint32_t hal_micros(void) {
    return board_stm32g474_micros();
}

void hal_delay_ms(uint32_t ms) {
    board_stm32g474_delay_ms(ms);
}

size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    return board_stm32g474_uart_read(dst, cap);
}

size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    return board_stm32g474_uart_write(src, len);
}

size_t hal_serial_write_str(hal_port_t port, const char *s) {
    if (!s) {
        return 0u;
    }
    return hal_serial_write(port, (const uint8_t *)s, strlen(s));
}

size_t hal_serial_encode32(hal_port_t port, const char *s) {
    return hal_serial_write_str(port, s);
}

void hal_gpio_write(uint32_t pin_id, hal_pin_state_t state) {
    (void)pin_id;
    (void)state;
}

hal_pin_state_t hal_gpio_read(uint32_t pin_id) {
    (void)pin_id;
    return HAL_PIN_LOW;
}

void hal_stepper_enable(bool en) {
    board_stm32g474_stepper_enable(en);
}

void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive) {
    board_stm32g474_stepper_set_dir(axis, dir_positive);
}

void hal_stepper_step_pulse(hal_axis_t axis) {
    board_stm32g474_stepper_pulse_mask(1u << axis);
}

void hal_stepper_step_clear(hal_axis_t axis) {
    board_stm32g474_stepper_step_clear(axis);
}

void hal_stepper_pulse_mask(uint32_t axis_mask) {
    board_stm32g474_stepper_pulse_mask(axis_mask);
}

void hal_spindle_set(hal_spindle_dir_t dir, float pwm_0_to_1) {
    (void)dir;
    (void)pwm_0_to_1;
}

void hal_coolant_mist(bool on) {
    (void)on;
}

void hal_coolant_flood(bool on) {
    (void)on;
}

void hal_read_inputs(hal_inputs_t *out) {
    board_stm32g474_read_inputs(out);
}

void hal_poll(void) {
    board_stm32g474_poll();
}

void hal_tick_1khz_isr(void) {
    board_stm32g474_tick_1khz_isr();
}
