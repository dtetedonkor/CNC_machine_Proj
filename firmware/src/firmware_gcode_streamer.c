#include "firmware_gcode_streamer.h"

#include <string.h>

static void enqueue_line(serial_uart_t *uart, const char *s) {
    if (!uart || !s) return;
    (void)serial_uart_tx_enqueue(uart, (const uint8_t *)s, strlen(s));
    (void)serial_uart_tx_enqueue(uart, (const uint8_t *)"\r\n", 2u);
}

void fw_gcode_streamer_init(fw_gcode_streamer_t *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));

    serial_uart_init(&s->uart);
    serial_gcode_bridge_init(&s->bridge);

    /* Optional: if you want to override motion backend:
     * serial_gcode_bridge_set_motion_backend(&s->bridge, my_backend, my_ctx);
     */
}

void fw_gcode_streamer_rx_bytes(fw_gcode_streamer_t *s, const uint8_t *data, size_t len) {
    if (!s || !data || len == 0u) return;

    /* Push into RX ring buffer (safe to call from ISR if you avoid races with pop;
     * for a real MCU you may want to guard with IRQ disable or make head/tail volatile).
     */
    (void)serial_uart_rx_push(&s->uart, data, len);
}

void fw_gcode_streamer_poll(fw_gcode_streamer_t *s) {
    if (!s) return;

    /* Keep draining lines that are already complete in the RX ring. */
    while (1) {
        uart_line_status_t lst = serial_uart_read_line(&s->uart, s->line_buf, sizeof(s->line_buf));
        if (lst == UART_LINE_NONE) {
            break;
        }

        if (lst == UART_LINE_OVERFLOW) {
            enqueue_line(&s->uart, "error: line overflow");
            continue;
        }

        /* Process one complete G-code line (already stripped of CR/LF). */
        s->resp_buf[0] = '\0';
        (void)serial_gcode_bridge_process_line(&s->bridge,
                                              s->line_buf,
                                              s->resp_buf,
                                              sizeof(s->resp_buf));

        /* Typical CNC streaming behavior: respond per line. */
        enqueue_line(&s->uart, s->resp_buf[0] ? s->resp_buf : "OK");
    }
}

bool fw_gcode_streamer_tx_pop(fw_gcode_streamer_t *s, uint8_t *out_byte) {
    if (!s) return false;
    return serial_uart_tx_pop_byte(&s->uart, out_byte);
}

size_t fw_gcode_streamer_tx_write(fw_gcode_streamer_t *s, const char *str) {
    if (!s || !str) return 0u;
    return serial_uart_tx_enqueue(&s->uart, (const uint8_t *)str, strlen(str));
}
