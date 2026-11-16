#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        // Trailing newline ('\n') hatayein agar read_cmd ne lagaya hai
        size_t len = strlen(cmdline);
        if (len > 0 && cmdline[len - 1] == '\n') {
            cmdline[len - 1] = '\0';
        }

        // Skip empty lines
        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Task 4: !n Re-execution check (tokenization aur history mein add karne se pehle)
        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            char* hist_cmd = get_history_cmd(n);
            if (hist_cmd == NULL) {
                fprintf(stderr, "Event !%d not found in history.\n", n);
                free(cmdline);
                continue;
            }
            printf("%s\n", hist_cmd); // Re-execute hone wali command screen par dikhayein
            free(cmdline);
            cmdline = hist_cmd; // Purani !n command ko retrieved command se replace karein
        }

        // Task 2: Command ko history mein add karein
        add_history_cmd(cmdline);

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
