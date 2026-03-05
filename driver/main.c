#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../src/hal.h"
#include "../src/serial_gcode_bridge.h"
#include "../src/serial_uart.h"

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

    write_line("CNC ready");

    while (1) {
        uint8_t rx_buf[32];
        const size_t rx = hal_serial_read(HAL_PORT_GCODE, rx_buf, sizeof(rx_buf));
        if (rx > 0u) {
            serial_uart_rx_push(&uart, rx_buf, rx);
        }

        char line[UART_LINE_MAX + 1];
        const uart_line_status_t line_status = serial_uart_read_line(&uart, line, sizeof(line));
        if (line_status == UART_LINE_READY) {
            char response[80];
            serial_gcode_bridge_process_line(&bridge, line, response, sizeof(response));
            write_line(response);
        } else if (line_status == UART_LINE_OVERFLOW) {
            write_line("error: line overflow");
        }

        hal_poll();
    }
}
