#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/core.h"

static bool mock_motor_enabled = false;
static uint32_t mock_pulse_counts[HAL_AXIS_MAX];
static uint32_t mock_time_us = 0u;
static hal_inputs_t mock_inputs = {0};

hal_status_t hal_init(void) { return HAL_OK; }
void hal_start(void) {}
void hal_deinit(void) {}
uint32_t hal_millis(void) { return mock_time_us / 1000u; }
uint32_t hal_micros(void) { return mock_time_us; }
void hal_delay_ms(uint32_t ms) { mock_time_us += ms * 1000u; }
size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    (void)dst;
    (void)cap;
    return 0u;
}
size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    (void)src;
    return len;
}
size_t hal_serial_write_str(hal_port_t port, const char *s) {
    return hal_serial_write(port, (const uint8_t *)s, s ? strlen(s) : 0u);
}
size_t hal_serial_encode32(hal_port_t port, const char *s) {
    (void)port;
    return s ? strlen(s) : 0u;
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
    (void)axis;
    (void)dir_positive;
}
void hal_stepper_step_pulse(hal_axis_t axis) { mock_pulse_counts[axis]++; }
void hal_stepper_step_clear(hal_axis_t axis) { (void)axis; }
void hal_stepper_pulse_mask(uint32_t axis_mask) {
    for (hal_axis_t axis = HAL_AXIS_X; axis < HAL_AXIS_MAX; axis++) {
        if ((axis_mask & (1u << axis)) != 0u) {
            mock_pulse_counts[axis]++;
        }
    }
}
void hal_spindle_set(hal_spindle_dir_t dir, float pwm_0_to_1) {
    (void)dir;
    (void)pwm_0_to_1;
}
void hal_coolant_mist(bool on) { (void)on; }
void hal_coolant_flood(bool on) { (void)on; }
void hal_read_inputs(hal_inputs_t *out) {
    if (out) {
        *out = mock_inputs;
    }
}
void hal_poll(void) { mock_time_us++; }
void hal_tick_1khz_isr(void) {}

static void reset_mocks(void) {
    mock_motor_enabled = false;
    memset(mock_pulse_counts, 0, sizeof(mock_pulse_counts));
    memset(&mock_inputs, 0, sizeof(mock_inputs));
    mock_time_us = 0u;
}

static void test_core_submit_line_valid_motion(void) {
    reset_mocks();
    core_t core;
    core_init(&core);

    char response[64];
    const gcode_status_t st = core_submit_line(&core, "G0 X1", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_motor_enabled);
    assert(mock_pulse_counts[HAL_AXIS_X] > 0u);
}

static void test_core_submit_line_invalid_gcode(void) {
    reset_mocks();
    core_t core;
    core_init(&core);

    char response[64];
    const gcode_status_t st = core_submit_line(&core, "G99", response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strncmp(response, "error:", 6) == 0);
}

static void test_core_submit_line_malformed_rejected_by_protocol(void) {
    reset_mocks();
    core_t core;
    core_init(&core);

    char malformed[] = {'G', '1', ' ', 'X', 0x01u, '\0'};
    char response[64];
    const gcode_status_t st = core_submit_line(&core, malformed, response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strcmp(response, "error: bad character") == 0);
}

static void test_core_limits_and_estop_status(void) {
    reset_mocks();
    core_t core;
    core_init(&core);

    mock_inputs.limit_x = true;
    mock_inputs.estop = true;

    char status[64];
    const size_t written = core_get_status(&core, status, sizeof(status));
    assert(written > 0u);
    assert(strstr(status, "ALARM") != NULL);
    assert(strstr(status, "ESTOP:1") != NULL);
}

static void test_core_submit_line_blocked_when_safety_active(void) {
    reset_mocks();
    core_t core;
    core_init(&core);

    mock_inputs.limit_x = true;

    char response[64];
    const gcode_status_t st = core_submit_line(&core, "G0 X1", response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strcmp(response, "error: safety input active") == 0);
    assert(!mock_motor_enabled);
}

int main(void) {
    printf("Running core integration tests...\n");
    test_core_submit_line_valid_motion();
    test_core_submit_line_invalid_gcode();
    test_core_submit_line_malformed_rejected_by_protocol();
    test_core_limits_and_estop_status();
    test_core_submit_line_blocked_when_safety_active();
    printf("All core integration tests passed!\n");
    return 0;
}
