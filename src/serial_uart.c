#include "serial_uart.h"

#include <string.h>

static uint16_t ring_next(uint16_t idx, uint16_t cap) {
    idx++;
    return (idx >= cap) ? 0u : idx;
}

void serial_uart_init(serial_uart_t *uart) {
    if (!uart) return;
    memset(uart, 0, sizeof(*uart));
}

bool serial_uart_rx_push_byte(serial_uart_t *uart, uint8_t byte) {
    if (!uart || uart->rx_count >= UART_RX_BUFFER_SIZE) return false;
    uart->rx_buf[uart->rx_tail] = byte;
    uart->rx_tail = ring_next(uart->rx_tail, UART_RX_BUFFER_SIZE);
    uart->rx_count++;
    return true;
}

size_t serial_uart_rx_push(serial_uart_t *uart, const uint8_t *data, size_t len) {
    if (!uart || !data) return 0u;
    size_t accepted = 0u;
    for (size_t i = 0; i < len; i++) {
        if (!serial_uart_rx_push_byte(uart, data[i])) break;
        accepted++;
    }
    return accepted;
}

bool serial_uart_rx_pop_byte(serial_uart_t *uart, uint8_t *out_byte) {
    if (!uart || !out_byte || uart->rx_count == 0u) return false;
    *out_byte = uart->rx_buf[uart->rx_head];
    uart->rx_head = ring_next(uart->rx_head, UART_RX_BUFFER_SIZE);
    uart->rx_count--;
    return true;
}

static void uart_finalize_line(serial_uart_t *uart, char *out_line, size_t out_cap) {
    if (!out_line || out_cap == 0u) return;
    if (uart->line_len >= out_cap) {
        memcpy(out_line, uart->line_buf, out_cap - 1u);
        out_line[out_cap - 1u] = '\0';
    } else {
        memcpy(out_line, uart->line_buf, uart->line_len);
        out_line[uart->line_len] = '\0';
    }
}

uart_line_status_t serial_uart_read_line(serial_uart_t *uart, char *out_line, size_t out_cap) {
    if (!uart || !out_line || out_cap == 0u) return UART_LINE_NONE;

    uint8_t byte = 0u;
    while (serial_uart_rx_pop_byte(uart, &byte)) {
        if (byte == '\r') {
            continue;
        }

        if (byte == '\n') {
            if (uart->line_len == 0u && !uart->line_overflow) {
                continue;
            }

            uart_finalize_line(uart, out_line, out_cap);
            uart_line_status_t st = uart->line_overflow ? UART_LINE_OVERFLOW : UART_LINE_READY;

            uart->line_len = 0u;
            uart->line_overflow = false;
            uart->line_buf[0] = '\0';
            return st;
        }

        if (byte == 0x08u || byte == 0x7Fu) {
            if (uart->line_len > 0u) {
                uart->line_len--;
                uart->line_buf[uart->line_len] = '\0';
            }
            continue;
        }

        if (byte < 0x20u || byte > 0x7Eu) {
            continue;
        }

        if (uart->line_len < UART_LINE_MAX) {
            uart->line_buf[uart->line_len++] = (char)byte;
            uart->line_buf[uart->line_len] = '\0';
        } else {
            uart->line_overflow = true;
        }
    }

    return UART_LINE_NONE;
}

size_t serial_uart_tx_enqueue(serial_uart_t *uart, const uint8_t *data, size_t len) {
    if (!uart || !data) return 0u;
    size_t accepted = 0u;
    for (size_t i = 0; i < len; i++) {
        if (uart->tx_count >= UART_TX_BUFFER_SIZE) break;
        uart->tx_buf[uart->tx_tail] = data[i];
        uart->tx_tail = ring_next(uart->tx_tail, UART_TX_BUFFER_SIZE);
        uart->tx_count++;
        accepted++;
    }
    return accepted;
}

bool serial_uart_tx_pop_byte(serial_uart_t *uart, uint8_t *out_byte) {
    if (!uart || !out_byte || uart->tx_count == 0u) return false;
    *out_byte = uart->tx_buf[uart->tx_head];
    uart->tx_head = ring_next(uart->tx_head, UART_TX_BUFFER_SIZE);
    uart->tx_count--;
    return true;
}
