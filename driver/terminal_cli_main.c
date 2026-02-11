#include "terminal_cli.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    terminal_cli_t cli;
    terminal_cli_init(&cli);

    char line[256];
    char response[256];

    printf("CNC terminal CLI (type 'quit' to exit)\n");

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        terminal_cli_process_line(&cli, line, response, sizeof(response));
        printf("%s\n", response);
    }

    return 0;
}
