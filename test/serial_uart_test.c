#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/serial_uart.h"

int main(void) {
    printf("Running serial UART transport tests...\n");

    serial_uart_t uart;
    serial_uart_init(&uart);

    const uint8_t line_stream[] = {'G', '1', ' ', 'X', '1', '0', '\r', '\n'};
    assert(serial_uart_rx_push(&uart, line_stream, sizeof(line_stream)) == sizeof(line_stream));

    char line[UART_LINE_MAX + 1];
    assert(serial_uart_read_line(&uart, line, sizeof(line)) == UART_LINE_READY);
    assert(strcmp(line, "G1 X10") == 0);

    const uint8_t backspace_stream[] = {'G', '1', ' ', 'X', '1', '2', 0x08u, '3', '\n'};
    assert(serial_uart_rx_push(&uart, backspace_stream, sizeof(backspace_stream)) == sizeof(backspace_stream));
    assert(serial_uart_read_line(&uart, line, sizeof(line)) == UART_LINE_READY);
    assert(strcmp(line, "G1 X13") == 0);

    uint8_t tx_data[] = {'o', 'k', '\n'};
    assert(serial_uart_tx_enqueue(&uart, tx_data, sizeof(tx_data)) == sizeof(tx_data));
    uint8_t out = 0u;
    assert(serial_uart_tx_pop_byte(&uart, &out) && out == 'o');
    assert(serial_uart_tx_pop_byte(&uart, &out) && out == 'k');
    assert(serial_uart_tx_pop_byte(&uart, &out) && out == '\n');
    assert(!serial_uart_tx_pop_byte(&uart, &out));

    printf("All serial UART tests passed!\n");
    return 0;
}
