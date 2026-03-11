// interactive_console_sim.c
// Build: gcc -Wall -Werror -pedantic -std=c99 -g interactive_console_sim.c -o interactive_console_sim
// Run:   ./interactive_console_sim

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

static void trim_crlf(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

// very small “parser” just for the demo
static bool is_simple_cmd(const char *line, const char *cmd) {
    // match whole token, allowing leading/trailing spaces
    while (isspace((unsigned char)*line)) line++;
    size_t cmd_len = strlen(cmd);
    if (strncmp(line, cmd, cmd_len) != 0) return false;
    line += cmd_len;
    while (isspace((unsigned char)*line)) line++;
    return *line == '\0';
}

int main(void) {
    bool motor_enabled = false;

    puts("POSIX serial simulation (interactive)");
    puts("MCU -> HOST: CNC ready");
    puts("Type G-code lines and press Enter. Type 'quit' to exit.\n");

    // ---- main loop: wait for user input ----
    for (;;) {
        char line[256];

        fputs("> ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            // EOF (Ctrl+D) or read error
            puts("\nSimulation done (EOF).");
            break;
        }

        trim_crlf(line);

        if (line[0] == '\0') {
            continue; // ignore empty lines
        }

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("\nSimulation done. motor_enabled=%s\n", motor_enabled ? "true" : "false");
            break;
        }

        // what your posix sim prints when it receives a line
        printf("HOST -> MCU: %s\n", line);

        // per-line counters (like your sim resets)
        unsigned dir_x = 0, dir_y = 0, dir_z = 0, dir_a = 0;
        unsigned pulse_x = 0, pulse_y = 0, pulse_z = 0, pulse_a = 0;

        // minimal behavior to mimic your repo’s transcript style
        if (is_simple_cmd(line, "M17")) {
            motor_enabled = true;
            puts("MCU [stepper]: enable=true");
            // no motion -> pulses stay 0
            puts("MCU [stepper]: dir_changes[X=0 Y=0 Z=0 A=0] pulses[X=0 Y=0 Z=0 A=0]");
            puts("MCU -> HOST: OK\n");
            continue;
        }

        if (is_simple_cmd(line, "M18")) {
            motor_enabled = false;
            puts("MCU [stepper]: enable=false");
            puts("MCU [stepper]: dir_changes[X=0 Y=0 Z=0 A=0] pulses[X=0 Y=0 Z=0 A=0]");
            puts("MCU -> HOST: OK\n");
            continue;
        }

        // If motors are disabled, you can choose to still say OK or emit an error.
        // Here we just say OK and show zero motion.
        if (!motor_enabled) {
            puts("MCU [stepper]: dir_changes[X=0 Y=0 Z=0 A=0] pulses[X=0 Y=0 Z=0 A=0]");
            puts("MCU -> HOST: OK\n");
            continue;
        }

        // Placeholder: fake that some motion happened for any other command while enabled
        // (You can later replace this with real parsing + step generation.)
        dir_x = 1; dir_y = 1;
        pulse_x = 80; pulse_y = 80;

        printf("MCU [stepper]: dir_changes[X=%u Y=%u Z=%u A=%u] pulses[X=%u Y=%u Z=%u A=%u]\n",
               dir_x, dir_y, dir_z, dir_a, pulse_x, pulse_y, pulse_z, pulse_a);
        puts("MCU -> HOST: OK\n");
    }

    return 0;
}