#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // Default filename completion enable rehta hai, explicitly bind karne ke liye:
    rl_bind_key('\t', rl_complete);

    while ((cmdline = readline(PROMPT)) != NULL) {
        // Skip empty lines (readline trailing '\n' khud hi hata deta hai)
        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Feature-3: !n Re-execution check
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

        // Dono history systems update karein:
        add_history(cmdline);      // GNU Readline (Up/Down arrow keys ke liye)
        add_history_cmd(cmdline);  // Feature-3 (custom 'history' aur '!n' ke liye)

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
