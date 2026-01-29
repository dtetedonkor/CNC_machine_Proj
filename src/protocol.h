/* protocol.h - Core CNC protocol layer (byte stream -> realtime events + G-code lines)
 *
 * Drop-in firmware module:
 *  - Feed bytes from UART/USB ISR or driver
 *  - Realtime commands are handled immediately
 *  - Full lines are normalized + queued + delivered via callback
 *
 * Design goals: small, predictable, no malloc, portable C99.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PROTOCOL_LINE_MAX
#define PROTOCOL_LINE_MAX 96u   /* Max length of a single G-code line (excluding '\0') */
#endif

#ifndef PROTOCOL_LINE_QUEUE_DEPTH
#define PROTOCOL_LINE_QUEUE_DEPTH 8u  /* Number of complete lines to buffer */
#endif

/* Realtime commands (modeled after common CNC controllers like Grbl). */
typedef enum {
    PROTO_RT_NONE = 0,
    PROTO_RT_STATUS_QUERY,     /* '?' */
    PROTO_RT_FEED_HOLD,        /* '!' */
    PROTO_RT_CYCLE_START,      /* '~' */
    PROTO_RT_RESET,            /* Ctrl-X (0x18) */
    PROTO_RT_ESTOP,            /* Optional: user-defined emergency stop */
} proto_rt_cmd_t;

/* Line-level errors (not motion errors). */
typedef enum {
    PROTO_LINE_OK = 0,
    PROTO_LINE_EMPTY,          /* blank/only whitespace/comments */
    PROTO_LINE_OVERFLOW,       /* line exceeded PROTOCOL_LINE_MAX */
    PROTO_LINE_BAD_CHAR,       /* non-printable / unsupported characters */
} proto_line_status_t;

/* Callback when a complete (normalized) line is ready. */
typedef void (*proto_line_cb_t)(const char *line, proto_line_status_t st, void *user);

/* Callback when a realtime command is received. */
typedef void (*proto_rt_cb_t)(proto_rt_cmd_t cmd, void *user);

/* Protocol configuration. */
typedef struct {
    bool strip_semicolon_comments; /* strip '; ...' */
    bool strip_paren_comments;     /* strip '( ... )' (not nested) */
    bool allow_dollar_commands;    /* allow lines starting with '$' to pass through */
    bool to_uppercase;            /* optional: normalize to uppercase */
} proto_config_t;

typedef struct protocol protocol_t;

/* Initialize protocol instance (zero dynamic allocation). */
void protocol_init(protocol_t *p,
                   const proto_config_t *cfg,
                   proto_line_cb_t on_line,
                   proto_rt_cb_t on_rt,
                   void *user);

/* Reset internal state: clears current line + queued lines. */
void protocol_reset(protocol_t *p);

/* Feed bytes into the protocol (call from ISR or driver). */
void protocol_feed_bytes(protocol_t *p, const uint8_t *data, size_t len);

/* Optional: poll to deliver queued lines outside ISR context.
 * If you call protocol_feed_bytes() in ISR, set callbacks to NULL and
 * pull lines in the main loop with protocol_pop_line().
 */
bool protocol_pop_line(protocol_t *p, char *out, size_t out_cap, proto_line_status_t *st);

/* Utility: returns true if there are pending completed lines buffered. */
bool protocol_has_line(const protocol_t *p);

#ifdef __cplusplus
}
#endif