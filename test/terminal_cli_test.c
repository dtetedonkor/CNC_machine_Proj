#include "../src/terminal_cli.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_terminal_cli_ok_responses(void) {
    printf("Testing terminal CLI OK responses...\n");

    terminal_cli_t cli;
    terminal_cli_init(&cli);

    char response[128];
    gcode_status_t status = terminal_cli_process_line(&cli, "G90", response, sizeof(response));
    assert(status == GCODE_OK);
    assert(strcmp(response, "OK J0=0.000 J1=0.000 J2=0.000") == 0);

    status = terminal_cli_process_line(&cli, "G01 X10 Y5 F200", response, sizeof(response));
    assert(status == GCODE_OK);
    assert(strcmp(response, "OK J0=15.000 J1=5.000 J2=0.000") == 0);

    printf("[passed]\n");
}

static void test_terminal_cli_error_response(void) {
    printf("Testing terminal CLI error response...\n");

    terminal_cli_t cli;
    terminal_cli_init(&cli);

    char response[128];
    gcode_status_t status = terminal_cli_process_line(&cli, "G01 X10", response, sizeof(response));
    assert(status == GCODE_ERR_MISSING_PARAM);
    assert(strcmp(response, "error: Missing parameter") == 0);

    printf("[passed]\n");
}

int main(void) {
    test_terminal_cli_ok_responses();
    test_terminal_cli_error_response();

    printf("All terminal CLI tests passed.\n");
    return 0;
}
