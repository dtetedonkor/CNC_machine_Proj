/* hal.h - Hardware Abstraction Layer entry point for CNC firmware core
 *
 * Core modules must only include this header (and other core headers),
 * never vendor SDK headers (stm32xxxx_hal.h, etc).
 *
 * Implement these functions in hal_<platform>.c (e.g., hal_stm32.c).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- Common types ----------------------------- */

typedef enum {
    HAL_OK = 0,
    HAL_ERR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} hal_status_t;

typedef enum {
    HAL_PIN_LOW = 0,
    HAL_PIN_HIGH = 1
} hal_pin_state_t;

/* Optional: identify which serial port is used for G-code streaming/console */
typedef enum {
    HAL_PORT_GCODE = 0,
    HAL_PORT_AUX   = 1
} hal_port_t;

/* Axis indexing for step/dir outputs */
typedef enum {
    HAL_AXIS_X = 0,
    HAL_AXIS_Y = 1,
    HAL_AXIS_Z = 2,
    HAL_AXIS_A = 3,
    HAL_AXIS_MAX
} hal_axis_t;

/* ----------------------------- Core lifecycle ----------------------------- */

/* Early board bring-up:
 * - clocks
 * - GPIO init
 * - UART/USB init
 * - timers used by systick/micros
 */
hal_status_t hal_init(void);

/* Optional: called right before main loop starts (after core init). */
void hal_start(void);

/* Optional: clean shutdown (rare on MCU, useful on simulator). */
void hal_deinit(void);

/* ----------------------------- Time base ----------------------------- */

/* Milliseconds since boot (must be monotonic). */
uint32_t hal_millis(void);

/* Microseconds since boot (monotonic if possible; can roll over). */
uint32_t hal_micros(void);

/* Busy delay. Keep short; core should prefer scheduling. */
void hal_delay_ms(uint32_t ms);

/* ----------------------------- Serial I/O ----------------------------- */

/* Non-blocking read:
 * Returns number of bytes written into dst (0 if none).
 * Safe to call from main loop.
 */
size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap);

/* Write bytes. May be blocking or buffered, your choice.
 * Return number of bytes accepted (0..len).
 */
size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len);

/* Optional convenience: write null-terminated string. */
size_t hal_serial_write_str(hal_port_t port, const char *s);

/* Base32 encoding for serial comms*/

size_t hal_serial_encode32(hal_port_t port, const char *s);
/* ----------------------------- Digital I/O ----------------------------- */

/* Generic GPIO (for LEDs, enables, etc.). */
void hal_gpio_write(uint32_t pin_id, hal_pin_state_t state);
hal_pin_state_t hal_gpio_read(uint32_t pin_id);

/* ----------------------------- Motion outputs ----------------------------- */

/* Step pulses:
 * The core planner/stepper module will call these.
 *
 * Recommended approach:
 * - hal_stepper_set_dir() sets direction pins
 * - hal_stepper_step_pulse() emits a single pulse (or sets step high)
 * - hal_stepper_step_clear() clears step pins (or sets step low)
 *
 * Implementation can be:
 * - bit-banged GPIO
 * - timer-driven (preferred), where these calls enqueue events
 */
void hal_stepper_enable(bool en);
void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive);

/* Set step pin(s) high for an axis. */
void hal_stepper_step_pulse(hal_axis_t axis);

/* Set step pin(s) low for an axis. */
void hal_stepper_step_clear(hal_axis_t axis);

/* Optional: atomic multi-axis pulse for tighter timing (bitmask). */
void hal_stepper_pulse_mask(uint32_t axis_mask);

/* ----------------------------- Spindle / coolant ----------------------------- */

typedef enum {
    HAL_SPINDLE_OFF = 0,
    HAL_SPINDLE_CW  = 1,
    HAL_SPINDLE_CCW = 2
} hal_spindle_dir_t;

/* PWM range is normalized 0.0 .. 1.0 (core doesn't care about timer resolution). */
void hal_spindle_set(hal_spindle_dir_t dir, float pwm_0_to_1);

/* Optional coolant. */
void hal_coolant_mist(bool on);
void hal_coolant_flood(bool on);

/* ----------------------------- Limit / control inputs ----------------------------- */

typedef struct {
    bool limit_x;
    bool limit_y;
    bool limit_z;
    bool estop;      /* physical e-stop input */
    bool probe;      /* touch probe input */
} hal_inputs_t;

void hal_read_inputs(hal_inputs_t *out);

/* ----------------------------- Scheduling hook ----------------------------- */

/* Called frequently from main loop; do platform polling here (USB, DMA flush, etc). */
void hal_poll(void);

/* Called from a fixed-rate timer ISR (e.g., 1kHz) if you want.
 * If you don't use it, just leave it uncalled in your platform layer.
 */
void hal_tick_1khz_isr(void);

#ifdef __cplusplus
}
#endif
