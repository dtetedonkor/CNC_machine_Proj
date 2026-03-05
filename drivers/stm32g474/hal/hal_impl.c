#include "../../../src/hal.h"

#include "../board/board_init.h"
#include "../board/board_pins.h"
#include "../periph/drv_gpio.h"
#include "../periph/drv_inputs.h"
#include "../periph/drv_stepper_timer.h"
#include "../periph/drv_timebase.h"
#include "../periph/drv_uart.h"

#include <stddef.h>

__attribute__((weak)) uint32_t core_step_tick_isr(void) {
    return 0;
}

hal_status_t hal_init(void) {
    board_init_clocks();
    drv_gpio_init_safe();
    drv_timebase_init();
    drv_uart_init();
    drv_inputs_init();
    drv_stepper_timer_init(BOARD_STEP_TICK_HZ, core_step_tick_isr);
    return HAL_OK;
}

void hal_start(void) {
    drv_stepper_timer_start();
}

void hal_deinit(void) {
    drv_stepper_timer_stop();
}

uint32_t hal_millis(void) {
    return drv_timebase_millis();
}

uint32_t hal_micros(void) {
    return drv_timebase_micros();
}

void hal_delay_ms(uint32_t ms) {
    drv_timebase_delay_ms(ms);
}

size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    return drv_uart_read(dst, cap);
}

size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    return drv_uart_write(src, len);
}

size_t hal_serial_write_str(hal_port_t port, const char *s) {
    size_t len = 0;

    if (s == NULL) {
        return 0;
    }

    while (s[len] != '\0') {
        len++;
    }

    return hal_serial_write(port, (const uint8_t *)s, len);
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
    drv_gpio_stepper_enable(en);
}

void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive) {
    drv_gpio_set_dir((uint8_t)axis, dir_positive);
}

void hal_stepper_step_pulse(hal_axis_t axis) {
    drv_gpio_emit_step_mask(1u << (uint8_t)axis);
}

void hal_stepper_step_clear(hal_axis_t axis) {
    drv_gpio_clear_step_mask(1u << (uint8_t)axis);
}

void hal_stepper_pulse_mask(uint32_t axis_mask) {
    drv_gpio_emit_step_mask(axis_mask);
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
    if (out == NULL) {
        return;
    }

    const drv_inputs_snapshot_t snapshot = drv_inputs_get();
    out->limit_x = (snapshot.state & BOARD_INPUT_BIT_LIMIT_X) != 0u;
    out->limit_y = (snapshot.state & BOARD_INPUT_BIT_LIMIT_Y) != 0u;
    out->limit_z = (snapshot.state & BOARD_INPUT_BIT_LIMIT_Z) != 0u;
    out->estop = (snapshot.state & BOARD_INPUT_BIT_ESTOP) != 0u;
    out->probe = false;
}

void hal_poll(void) {
    drv_inputs_poll();
}

void hal_tick_1khz_isr(void) {
}
