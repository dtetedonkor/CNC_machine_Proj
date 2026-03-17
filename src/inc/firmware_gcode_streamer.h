#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "serial_uart.h"
#include "serial_gcode_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FW_GCODE_LINE_OUT_MAX
#define FW_GCODE_LINE_OUT_MAX 128u
#endif

#ifndef FW_GCODE_RESP_MAX
#define FW_GCODE_RESP_MAX 128u
#endif

/* Minimal "streamer" state:
 * - uart line assembler/ring buffers
 * - gcode bridge (parser + motion integration)
 */
typedef struct {
    serial_uart_t uart;
    serial_gcode_bridge_t bridge;

    char line_buf[FW_GCODE_LINE_OUT_MAX];
    char resp_buf[FW_GCODE_RESP_MAX];
} fw_gcode_streamer_t;

/* Initialize streamer. */
void fw_gcode_streamer_init(fw_gcode_streamer_t *s);

/* Feed incoming UART bytes (call from ISR or DMA callback). */
void fw_gcode_streamer_rx_bytes(fw_gcode_streamer_t *s, const uint8_t *data, size_t len);

/* Polling function (call frequently from main loop).
 * - pulls full lines
 * - runs gcode parser/motion
 * - enqueues "OK"/"error: ..." responses into TX buffer
 */
void fw_gcode_streamer_poll(fw_gcode_streamer_t *s);

/* Pull bytes that should be transmitted (your UART TX ISR/DMA can use this). */
bool fw_gcode_streamer_tx_pop(fw_gcode_streamer_t *s, uint8_t *out_byte);

/* Convenience: enqueue a raw string for TX (adds exactly what you pass). */
size_t fw_gcode_streamer_tx_write(fw_gcode_streamer_t *s, const char *str);

#ifdef __cplusplus
}
#endif
