#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "../src/hal.h"
#include "../src/serial_gcode_bridge.h"
#include "../src/serial_uart.h"
#include "../drivers/stm32g474/hal/axis_motion.h"

static axis_motion_state_t g_axis_motion;

uint32_t core_step_tick_isr(void) {
    return axis_motion_step_tick(&g_axis_motion);
}

static bool timer_motion_backend(void *ctx,
                                 float start_x,
                                 float start_y,
                                 float end_x,
                                 float end_y,
                                 const float *steps_per_mm,
                                 uint32_t step_pulse_delay_us) {
    (void)ctx;
    const float dx = end_x - start_x;
    const float dy = end_y - start_y;
    const int32_t x_steps = (int32_t)lroundf(fabsf(dx) * steps_per_mm[HAL_AXIS_X]);
    const int32_t y_steps = (int32_t)lroundf(fabsf(dy) * steps_per_mm[HAL_AXIS_Y]);

    if (!axis_motion_start_xy(&g_axis_motion,
                              x_steps,
                              y_steps,
                              dx >= 0.0f,
                              dy >= 0.0f,
                              step_pulse_delay_us)) {
        return false;
    }

    return axis_motion_wait_complete(&g_axis_motion);
}

static void write_line(const char *msg) {
    hal_serial_write_str(HAL_PORT_GCODE, msg);
    hal_serial_write_str(HAL_PORT_GCODE, "\r\n");
}

int main(void) {
    if (hal_init() != HAL_OK) {
        while (1) {
        }
    }
    hal_start();

    serial_uart_t uart;
    serial_uart_init(&uart);

    serial_gcode_bridge_t bridge;
    serial_gcode_bridge_init(&bridge);
    axis_motion_init(&g_axis_motion);
    serial_gcode_bridge_set_motion_backend(&bridge, timer_motion_backend, &g_axis_motion);

    write_line("CNC ready");
    uint32_t last_ready_ms = hal_millis();

    while (1) {
        if ((hal_millis() - last_ready_ms) >= 1000u) {
            last_ready_ms += 1000u;
            write_line("CNC ready");
        }

        uint8_t rx_buf[32];
        const size_t rx = hal_serial_read(HAL_PORT_GCODE, rx_buf, sizeof(rx_buf));
        if (rx > 0u) {
            serial_uart_rx_push(&uart, rx_buf, rx);
        }

        char line[UART_LINE_MAX + 1];
        const uart_line_status_t line_status = serial_uart_read_line(&uart, line, sizeof(line));
        if (line_status == UART_LINE_READY) {
            char response[80];
            const gcode_status_t status =
                serial_gcode_bridge_process_line(&bridge, line, response, sizeof(response));
            if (status != GCODE_OK) {
                hal_stepper_enable(false);
            }
            write_line(response);
        } else if (line_status == UART_LINE_OVERFLOW) {
            write_line("error: line overflow");
        }

        hal_poll();
    }
}
