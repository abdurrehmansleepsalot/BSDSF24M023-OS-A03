#include "shell.h"

int main() {
    char* cmdline;

    rl_bind_key('\t', rl_complete);

    while (1) {
        reap_zombies();

        cmdline = readline(PROMPT);
        if (cmdline == NULL) break;

        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // !n Re-execution check
        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            char* hist_cmd = get_history_cmd(n);
            if (hist_cmd == NULL) {
                fprintf(stderr, "Event !%d not found in history.\n", n);
                free(cmdline);
                continue;
            }
            printf("%s\n", hist_cmd);
            free(cmdline);
            cmdline = hist_cmd;
        }

        add_history(cmdline);
        add_history_cmd(cmdline);

        // Check karein ke command 'if ' se shuru ho rahi hai ya nahi
        char* check = cmdline;
        while (*check == ' ' || *check == '\t') check++;

        if (strncmp(check, "if ", 3) == 0) {
            handle_if_block(check);
        } else {
            run_single_line(cmdline);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
