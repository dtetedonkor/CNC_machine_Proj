#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/serial_gcode_bridge.h"
#include "../src/hal.h"
#include "../src/kinematics.h"

static bool mock_motor_enabled = false;
static uint32_t mock_pulse_counts[HAL_AXIS_MAX];
static uint32_t mock_dir_set_counts[HAL_AXIS_MAX];

hal_status_t hal_init(void) { return HAL_OK; }
void hal_start(void) {}
void hal_deinit(void) {}
uint32_t hal_millis(void) { return 0; }
uint32_t hal_micros(void) { return 0; }
void hal_delay_ms(uint32_t ms) { (void)ms; }
size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    (void)dst;
    (void)cap;
    return 0;
}
size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    (void)src;
    return len;
}
size_t hal_serial_write_str(hal_port_t port, const char *s) {
    (void)port;
    return (s != NULL) ? strlen(s) : 0u;
}
size_t hal_serial_encode32(hal_port_t port, const char *s) {
    (void)port;
    return (s != NULL) ? strlen(s) : 0u;
}
void hal_gpio_write(uint32_t pin_id, hal_pin_state_t state) {
    (void)pin_id;
    (void)state;
}
hal_pin_state_t hal_gpio_read(uint32_t pin_id) {
    (void)pin_id;
    return HAL_PIN_LOW;
}
void hal_stepper_enable(bool en) { mock_motor_enabled = en; }
void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive) {
    (void)dir_positive;
    mock_dir_set_counts[axis]++;
}
void hal_stepper_step_pulse(hal_axis_t axis) { mock_pulse_counts[axis]++; }
void hal_stepper_step_clear(hal_axis_t axis) { (void)axis; }
void hal_stepper_pulse_mask(uint32_t axis_mask) { (void)axis_mask; }
void hal_spindle_set(hal_spindle_dir_t dir, float pwm_0_to_1) {
    (void)dir;
    (void)pwm_0_to_1;
}
void hal_coolant_mist(bool on) { (void)on; }
void hal_coolant_flood(bool on) { (void)on; }
void hal_read_inputs(hal_inputs_t *out) {
    if (out) {
        memset(out, 0, sizeof(*out));
    }
}
void hal_poll(void) {}
void hal_tick_1khz_isr(void) {}

static void reset_mocks(void) {
    mock_motor_enabled = false;
    memset(mock_pulse_counts, 0, sizeof(mock_pulse_counts));
    memset(mock_dir_set_counts, 0, sizeof(mock_dir_set_counts));
}

static void test_g0_motion_emits_ok_and_steps(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G0 X1", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_motor_enabled);
    assert(mock_dir_set_counts[HAL_AXIS_X] > 0u);
    assert(mock_pulse_counts[HAL_AXIS_X] > 0u);
}

static void test_enable_disable_commands(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "M17", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_motor_enabled);

    st = serial_gcode_bridge_process_line(&bridge, "M18", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(!mock_motor_enabled);
}

static void test_invalid_line_returns_error(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G1 XBAD", response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strncmp(response, "error:", 6) == 0);
}

int main(void) {
    printf("Running serial gcode bridge tests...\n");
    test_g0_motion_emits_ok_and_steps();
    test_enable_disable_commands();
    test_invalid_line_returns_error();
    printf("All serial gcode bridge tests passed!\n");
    return 0;
}
