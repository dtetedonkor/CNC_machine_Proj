#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/hal.h"
#include "../src/serial_gcode_bridge.h"

#define SHELL_PROMPT "gcode> "
#define SHELL_INPUT_MAX 256u
#define SHELL_RESPONSE_MAX 80u

static uint32_t g_mock_time_us = 0u;
static char g_uart_line_buf[256];
static size_t g_uart_line_len = 0u;
static bool g_motor_enabled = false;
static uint32_t g_step_pulses[HAL_AXIS_MAX];
static uint32_t g_dir_changes[HAL_AXIS_MAX];

hal_status_t hal_init(void) { return HAL_OK; }
void hal_start(void) {}
void hal_deinit(void) {}
uint32_t hal_millis(void) { return g_mock_time_us / 1000u; }
uint32_t hal_micros(void) { return g_mock_time_us; }
void hal_delay_ms(uint32_t ms) { g_mock_time_us += ms * 1000u; }

size_t hal_serial_read(hal_port_t port, uint8_t *dst, size_t cap) {
    (void)port;
    (void)dst;
    (void)cap;
    return 0u;
}

size_t hal_serial_write(hal_port_t port, const uint8_t *src, size_t len) {
    (void)port;
    if (!src) {
        return 0u;
    }

    for (size_t i = 0; i < len; ++i) {
        const char c = (char)src[i];
        if (g_uart_line_len + 1u < sizeof(g_uart_line_buf) && c != '\r' && c != '\n') {
            g_uart_line_buf[g_uart_line_len++] = c;
            g_uart_line_buf[g_uart_line_len] = '\0';
        }
        if (c == '\n') {
            printf("MCU -> HOST: %s\n", g_uart_line_buf);
            g_uart_line_len = 0u;
            g_uart_line_buf[0] = '\0';
        }
    }
    return len;
}

size_t hal_serial_write_str(hal_port_t port, const char *s) {
    return hal_serial_write(port, (const uint8_t *)s, (s != NULL) ? strlen(s) : 0u);
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
    g_motor_enabled = en;
    printf("MCU [stepper]: enable=%s\n", en ? "true" : "false");
}

void hal_stepper_set_dir(hal_axis_t axis, bool dir_positive) {
    (void)dir_positive;
    g_dir_changes[axis]++;
}

void hal_stepper_step_pulse(hal_axis_t axis) {
    g_step_pulses[axis]++;
}

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
void hal_poll(void) { g_mock_time_us++; }
void hal_tick_1khz_isr(void) {}

static void write_line(const char *msg) {
    hal_serial_write_str(HAL_PORT_GCODE, msg);
    hal_serial_write_str(HAL_PORT_GCODE, "\r\n");
}

int main(void) {
    if (hal_init() != HAL_OK) {
        fprintf(stderr, "hal_init failed\n");
        return 1;
    }
    hal_start();

    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);

    printf("CNC G-code Shell\n");
    printf("Type G-code commands (e.g. G0 X10 Y20). Type 'quit' to exit.\n\n");
    write_line("CNC ready");

    char line[SHELL_INPUT_MAX];

    while (1) {
        printf(SHELL_PROMPT);
        fflush(stdout);

        if (!fgets(line, (int)sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        /* Strip trailing newline / carriage-return */
        size_t len = strlen(line);
        while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0u) {
            continue;
        }

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }

        memset(g_step_pulses, 0, sizeof(g_step_pulses));
        memset(g_dir_changes, 0, sizeof(g_dir_changes));

        char response[SHELL_RESPONSE_MAX];
        const gcode_status_t st =
            serial_gcode_bridge_process_line(&bridge, line, response, SHELL_RESPONSE_MAX);
        if (st != GCODE_OK) {
            hal_stepper_enable(false);
        }

        printf("MCU [stepper]: dir_changes[X=%lu Y=%lu Z=%lu A=%lu]"
               " pulses[X=%lu Y=%lu Z=%lu A=%lu]\n",
               (unsigned long)g_dir_changes[HAL_AXIS_X],
               (unsigned long)g_dir_changes[HAL_AXIS_Y],
               (unsigned long)g_dir_changes[HAL_AXIS_Z],
               (unsigned long)g_dir_changes[HAL_AXIS_A],
               (unsigned long)g_step_pulses[HAL_AXIS_X],
               (unsigned long)g_step_pulses[HAL_AXIS_Y],
               (unsigned long)g_step_pulses[HAL_AXIS_Z],
               (unsigned long)g_step_pulses[HAL_AXIS_A]);
        write_line(response);

        hal_poll();
    }

    printf("Session ended. motor_enabled=%s\n", g_motor_enabled ? "true" : "false");
    return 0;
}
