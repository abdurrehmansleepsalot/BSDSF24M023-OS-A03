#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    rl_bind_key('\t', rl_complete);

    while (1) {
        // Task 4: Naya prompt dikhane se pehle finished background jobs ko reap karein
        reap_zombies();

        cmdline = readline(PROMPT);
        if (cmdline == NULL) break; // Ctrl+D

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

        // Task 2: Semicolon (;) par commands ko split kar ke sequentially chalayein
        char* saveptr;
        char* subcmd = strtok_r(cmdline, ";", &saveptr);
        while (subcmd != NULL) {
            if ((arglist = tokenize(subcmd)) != NULL) {
                if (!handle_builtin(arglist)) {
                    execute(arglist);
                }

                for (int i = 0; arglist[i] != NULL; i++) {
                    free(arglist[i]);
                }
                free(arglist);
            }
            subcmd = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
