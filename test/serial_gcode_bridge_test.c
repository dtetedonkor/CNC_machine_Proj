#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/serial_gcode_bridge.h"
#include "../src/serial_uart.h"
#include "../src/hal.h"
#include "../src/kinematics.h"

static bool mock_motor_enabled = false;
static uint32_t mock_pulse_counts[HAL_AXIS_MAX];
static uint32_t mock_dir_set_counts[HAL_AXIS_MAX];
static uint32_t mock_time_us = 0;
static hal_inputs_t mock_inputs = {0};
static uint32_t mock_pulse_mask_calls = 0u;
static uint8_t mock_rx_bytes[UART_RX_BUFFER_SIZE];
static size_t mock_rx_len = 0u;
static size_t mock_rx_pos = 0u;
static char mock_tx_text[512];
static size_t mock_tx_len = 0u;
static const size_t TEST_POLL_ITERATIONS_LIMIT = 16u;
static const size_t GCODE_RESPONSE_MAX_LEN = 80u;
static const float FLOAT_EPSILON = 0.0001f;
static const char DRIVER_READY_MSG[] = "CNC ready";
static const char DRIVER_READY_LINE[] = "CNC ready\r\n";
static uint32_t mock_motion_backend_calls = 0u;

hal_status_t hal_init(void) { return HAL_OK; }
void hal_start(void) {}
void hal_deinit(void) {}
uint32_t hal_millis(void) { return mock_time_us / 1000u; }
uint32_t hal_micros(void) { return mock_time_us; }
void hal_delay_ms(uint32_t ms) { mock_time_us += (ms * 1000u); }
size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    if (!dst || cap == 0u || mock_rx_pos >= mock_rx_len) {
        return 0u;
    }
    const size_t remaining = mock_rx_len - mock_rx_pos;
    const size_t to_copy = (remaining < cap) ? remaining : cap;
    memcpy(dst, &mock_rx_bytes[mock_rx_pos], to_copy);
    mock_rx_pos += to_copy;
    return to_copy;
}
size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    if (!src || len == 0u || mock_tx_len >= sizeof(mock_tx_text) - 1u) {
        return 0u;
    }
    const size_t remaining = (sizeof(mock_tx_text) - 1u) - mock_tx_len;
    const size_t to_copy = (len < remaining) ? len : remaining;
    memcpy(&mock_tx_text[mock_tx_len], src, to_copy);
    mock_tx_len += to_copy;
    mock_tx_text[mock_tx_len] = '\0';
    return to_copy;
}
size_t hal_serial_write_str(hal_port_t port, const char *s) {
    return hal_serial_write(port, (const uint8_t *)s, (s != NULL) ? strlen(s) : 0u);
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
void hal_stepper_pulse_mask(uint32_t axis_mask) {
    mock_pulse_mask_calls++;
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
    memset(mock_dir_set_counts, 0, sizeof(mock_dir_set_counts));
    mock_time_us = 0;
    mock_rx_len = 0u;
    mock_rx_pos = 0u;
    mock_tx_len = 0u;
    mock_tx_text[0] = '\0';
    memset(&mock_inputs, 0, sizeof(mock_inputs));
    mock_pulse_mask_calls = 0u;
    mock_motion_backend_calls = 0u;
}

static bool mock_motion_backend(void *ctx,
                                float start_x,
                                float start_y,
                                float end_x,
                                float end_y,
                                const float *steps_per_mm,
                                uint32_t step_pulse_delay_us) {
    (void)ctx;
    (void)start_x;
    (void)start_y;
    (void)end_x;
    (void)end_y;
    (void)steps_per_mm;
    (void)step_pulse_delay_us;
    mock_motion_backend_calls++;
    return true;
}

static void mock_uart_feed_text(const char *text) {
    assert(text != NULL);
    const size_t len = strlen(text);
    assert(len <= sizeof(mock_rx_bytes));
    memcpy(mock_rx_bytes, text, len);
    mock_rx_len = len;
    mock_rx_pos = 0u;
}

static void driver_write_line(const char *msg) {
    hal_serial_write_str(HAL_PORT_GCODE, msg);
    hal_serial_write_str(HAL_PORT_GCODE, "\r\n");
}

typedef struct {
    serial_uart_t uart;
    serial_gcode_bridge_t bridge;
    uint32_t last_ready_ms;
    bool awaiting_host_input;
} test_driver_context_t;

static void driver_runtime_init(test_driver_context_t *ctx) {
    assert(ctx != NULL);
    assert(hal_init() == HAL_OK);
    hal_start();
    serial_uart_init(&ctx->uart);
    serial_gcode_bridge_init(&ctx->bridge);
    driver_write_line(DRIVER_READY_MSG);
    ctx->last_ready_ms = hal_millis();
    ctx->awaiting_host_input = true;
}

static void driver_runtime_poll_once(test_driver_context_t *ctx) {
    assert(ctx != NULL);
    uint8_t rx_buf[32];
    const size_t rx = hal_serial_read(HAL_PORT_GCODE, rx_buf, sizeof(rx_buf));
    if (rx > 0u) {
        ctx->awaiting_host_input = false;
        serial_uart_rx_push(&ctx->uart, rx_buf, rx);
    }

    const uint32_t now_ms = hal_millis();
    if (ctx->awaiting_host_input && ((now_ms - ctx->last_ready_ms) >= 1000u)) {
        ctx->last_ready_ms = now_ms;
        driver_write_line(DRIVER_READY_MSG);
    }

    char line[UART_LINE_MAX + 1];
    const uart_line_status_t line_status = serial_uart_read_line(&ctx->uart, line, sizeof(line));
    if (line_status == UART_LINE_READY) {
        char response[GCODE_RESPONSE_MAX_LEN];
        const gcode_status_t status =
            serial_gcode_bridge_process_line(&ctx->bridge, line, response, sizeof(response));
        if (status != GCODE_OK) {
            hal_stepper_enable(false);
        }
        driver_write_line(response);
    } else if (line_status == UART_LINE_OVERFLOW) {
        driver_write_line("error: line overflow");
    }

    hal_poll();
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

static void test_identity_query_returns_firmware_info(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$I", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "[FW:STM32G4 CNC_machine_Proj]") == 0);
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

static void test_settings_dump_contains_required_entries_without_laser(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[2048];
    const gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$$", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "$0=") != NULL);
    assert(strstr(response, "$132=") != NULL);
    assert(strstr(response, "$32=") == NULL);
}

static void test_setting_assignment_updates_internal_values(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[128];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$100=123.5", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(fabsf(bridge.steps_per_mm[HAL_AXIS_X] - 123.5f) < FLOAT_EPSILON);

    st = serial_gcode_bridge_process_line(&bridge, "$0=9", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(bridge.step_pulse_delay_us == 9u);
    assert(bridge.settings.step_pulse_time_us == 9u);
}

static void test_setting_assignment_rejects_laser_and_invalid_values(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[128];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$32=1", response, sizeof(response));
    assert(st == GCODE_ERR_UNSUPPORTED_CMD);
    assert(strstr(response, "unsupported setting $32") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "$100=abc", response, sizeof(response));
    assert(st == GCODE_ERR_INVALID_PARAM);
    assert(strstr(response, "invalid value for setting $100") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "$999=1", response, sizeof(response));
    assert(st == GCODE_ERR_INVALID_PARAM);
    assert(strstr(response, "unknown setting $999") != NULL);
}

static void test_coordinate_offsets_and_modal_state_queries(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[512];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$#", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "[G54:") != NULL);
    assert(strstr(response, "[G59:") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "$G", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "[GC:") != NULL);
    assert(strstr(response, "G54") != NULL);
    assert(strstr(response, "G21") != NULL);
    assert(strstr(response, "G90") != NULL);
}

static void test_startup_lines_show_and_set(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[256];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$N", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "$N0=") != NULL);
    assert(strstr(response, "$N1=") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "$N0=G21", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    st = serial_gcode_bridge_process_line(&bridge, "$N1=G90", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);

    st = serial_gcode_bridge_process_line(&bridge, "$N", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "$N0=G21") != NULL);
    assert(strstr(response, "$N1=G90") != NULL);
}

static void test_check_mode_hold_and_resume_commands(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[128];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$C", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "check mode: ON") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "G0 X1", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_pulse_mask_calls == 0u);

    st = serial_gcode_bridge_process_line(&bridge, "!", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    st = serial_gcode_bridge_process_line(&bridge, "~", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
}

static void test_unlock_homing_and_soft_reset_commands(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[128];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "$X", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);

    st = serial_gcode_bridge_process_line(&bridge, "$H", response, sizeof(response));
    assert(st == GCODE_ERR_UNSUPPORTED_CMD);
    assert(strstr(response, "homing disabled") != NULL);

    st = serial_gcode_bridge_process_line(&bridge, "$22=1", response, sizeof(response));
    assert(st == GCODE_OK);
    st = serial_gcode_bridge_process_line(&bridge, "G0 X2 Y2", response, sizeof(response));
    assert(st == GCODE_OK);
    st = serial_gcode_bridge_process_line(&bridge, "$H", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);

    char reset_cmd[2];
    reset_cmd[0] = 0x18;
    reset_cmd[1] = '\0';
    st = serial_gcode_bridge_process_line(&bridge, reset_cmd, response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strstr(response, "Grbl reset") != NULL);
}

static void test_driver_startup_and_mock_gcode_over_uart(void) {
    reset_mocks();
    mock_uart_feed_text("M17\nG0 X1 Y1\nM18\n");

    test_driver_context_t runtime;
    driver_runtime_init(&runtime);
    for (size_t i = 0; i < TEST_POLL_ITERATIONS_LIMIT; ++i) {
        driver_runtime_poll_once(&runtime);
    }

    assert(strncmp(mock_tx_text, DRIVER_READY_LINE, strlen(DRIVER_READY_LINE)) == 0);
    assert(strstr(mock_tx_text, "OK\r\n") != NULL);
    assert(mock_pulse_counts[HAL_AXIS_X] > 0u);
    assert(mock_pulse_counts[HAL_AXIS_Y] > 0u);
    assert(!mock_motor_enabled);
    assert(mock_time_us > 0u);
}

static void test_driver_ready_prints_until_host_input(void) {
    reset_mocks();

    test_driver_context_t runtime;
    driver_runtime_init(&runtime);

    mock_time_us = 1000u * 1000u;
    driver_runtime_poll_once(&runtime);
    assert(strcmp(mock_tx_text, "CNC ready\r\nCNC ready\r\n") == 0);

    mock_uart_feed_text("M17\n");
    mock_time_us = 2000u * 1000u;
    driver_runtime_poll_once(&runtime);
    assert(strcmp(mock_tx_text, "CNC ready\r\nCNC ready\r\nOK\r\n") == 0);
}

static void test_xy_motion_uses_atomic_pulse_mask(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G0 X1 Y1", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_pulse_mask_calls > 0u);
    assert(mock_pulse_counts[HAL_AXIS_X] > 0u);
    assert(mock_pulse_counts[HAL_AXIS_Y] > 0u);
}

static void test_motion_aborts_on_estop_input(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    mock_inputs.estop = true;

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G0 X1", response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strstr(response, "safety input active") != NULL);
    assert(!mock_motor_enabled);
}

static void test_motion_aborts_on_limit_input(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    mock_inputs.limit_x = true;

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G0 X1", response, sizeof(response));
    assert(st != GCODE_OK);
    assert(strstr(response, "safety input active") != NULL);
    assert(!mock_motor_enabled);
}

static void test_custom_motion_backend_is_used(void) {
    reset_mocks();
    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);
    serial_gcode_bridge_set_motion_backend(&bridge, mock_motion_backend, NULL);

    char response[64];
    gcode_status_t st = serial_gcode_bridge_process_line(&bridge, "G0 X1", response, sizeof(response));
    assert(st == GCODE_OK);
    assert(strcmp(response, "OK") == 0);
    assert(mock_motion_backend_calls == 1u);
    assert(mock_pulse_mask_calls == 0u);
}

int main(void) {
    printf("Running serial gcode bridge tests...\n");
    test_g0_motion_emits_ok_and_steps();
    test_enable_disable_commands();
    test_identity_query_returns_firmware_info();
    test_invalid_line_returns_error();
    test_settings_dump_contains_required_entries_without_laser();
    test_setting_assignment_updates_internal_values();
    test_setting_assignment_rejects_laser_and_invalid_values();
    test_coordinate_offsets_and_modal_state_queries();
    test_startup_lines_show_and_set();
    test_check_mode_hold_and_resume_commands();
    test_unlock_homing_and_soft_reset_commands();
    test_driver_startup_and_mock_gcode_over_uart();
    test_driver_ready_prints_until_host_input();
    test_xy_motion_uses_atomic_pulse_mask();
    test_motion_aborts_on_estop_input();
    test_motion_aborts_on_limit_input();
    test_custom_motion_backend_is_used();
    printf("All serial gcode bridge tests passed!\n");
    return 0;
}
