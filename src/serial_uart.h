#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE 256u
#endif

#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE 256u
#endif

#ifndef UART_LINE_MAX
#define UART_LINE_MAX 120u
#endif

typedef enum {
    UART_LINE_NONE = 0,
    UART_LINE_READY,
    UART_LINE_OVERFLOW,
} uart_line_status_t;

typedef struct {
    uint8_t rx_buf[UART_RX_BUFFER_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t rx_count;

    uint8_t tx_buf[UART_TX_BUFFER_SIZE];
    uint16_t tx_head;
    uint16_t tx_tail;
    uint16_t tx_count;

    char line_buf[UART_LINE_MAX + 1];
    uint16_t line_len;
    bool line_overflow;
} serial_uart_t;

void serial_uart_init(serial_uart_t *uart);
bool serial_uart_rx_push_byte(serial_uart_t *uart, uint8_t byte);
size_t serial_uart_rx_push(serial_uart_t *uart, const uint8_t *data, size_t len);
bool serial_uart_rx_pop_byte(serial_uart_t *uart, uint8_t *out_byte);

uart_line_status_t serial_uart_read_line(serial_uart_t *uart, char *out_line, size_t out_cap);

size_t serial_uart_tx_enqueue(serial_uart_t *uart, const uint8_t *data, size_t len);
bool serial_uart_tx_pop_byte(serial_uart_t *uart, uint8_t *out_byte);

#ifdef __cplusplus
}
#endif
