#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/protocol.h"

typedef struct {
    proto_rt_cmd_t cmd;
    unsigned count;
} rt_capture_t;

static void on_rt(proto_rt_cmd_t cmd, void *user) {
    rt_capture_t *capture = (rt_capture_t *)user;
    capture->cmd = cmd;
    capture->count++;
}

int main(void) {
    printf("Running protocol tests...\n");

    protocol_t proto;
    rt_capture_t capture = {0};
    proto_config_t cfg = {
        .strip_semicolon_comments = true,
        .strip_paren_comments = true,
        .allow_dollar_commands = true,
        .to_uppercase = true,
    };

    protocol_init(&proto, &cfg, NULL, on_rt, &capture);

    const uint8_t mixed_stream[] = "g1 x1 y2 ;comment\r\n?\n";
    protocol_feed_bytes(&proto, mixed_stream, sizeof(mixed_stream) - 1u);

    char out[PROTOCOL_LINE_MAX + 1];
    proto_line_status_t status = PROTO_LINE_EMPTY;
    assert(protocol_pop_line(&proto, out, sizeof(out), &status));
    assert(status == PROTO_LINE_OK);
    assert(strcmp(out, "G1 X1 Y2") == 0);
    assert(capture.count == 1u);
    assert(capture.cmd == PROTO_RT_STATUS_QUERY);

    const uint8_t bad_char_stream[] = {'G', '1', ' ', 'X', 0x01u, '\n'};
    protocol_feed_bytes(&proto, bad_char_stream, sizeof(bad_char_stream));
    assert(protocol_pop_line(&proto, out, sizeof(out), &status));
    assert(status == PROTO_LINE_BAD_CHAR);
    assert(strcmp(out, "G1 X") == 0);

    const uint8_t hold_stream[] = {'!', '~', 0x18u};
    protocol_feed_bytes(&proto, hold_stream, sizeof(hold_stream));
    assert(capture.count == 4u);
    assert(capture.cmd == PROTO_RT_RESET);

    printf("All protocol tests passed!\n");
    return 0;
}
