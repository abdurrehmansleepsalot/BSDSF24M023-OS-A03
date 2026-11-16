#include "shell.h"
char* history[HISTORY_SIZE];
int history_count = 0;

void add_history_cmd(const char* cmd) {
    // Khali command ya sirf spaces ko history mein save na karein
    if (cmd == NULL || strlen(cmd) == 0) return;

    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        // Agar 20 commands poori ho jayein to sab se purani (index 0) free kar ke shift karein
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

char* get_history_cmd(int n) {
    if (n < 1 || n > history_count) {
        return NULL; // Out of bounds
    }
    return strdup(history[n - 1]); // Nayi copy return karein taake main loop mein safely free ho sake
}
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    // 1. exit command
    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    }

    // 2. cd <directory> command
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            // Agar user sirf 'cd' likhe to HOME directory mein jayein
            char* home = getenv("HOME");
            if (home == NULL || chdir(home) != 0) {
                perror("cd");
            }
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("cd"); // Agar directory exist na karti ho to error print karega
            }
        }
        return 1;
    }

    // 3. help command
    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd <dir>  - Change the current working directory\n");
        printf("  exit      - Terminate the shell\n");
        printf("  help      - Display this help message\n");
        printf("  jobs      - List active background jobs\n");
        return 1;
    }

    // 4. jobs command (placeholder)
    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }
    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    return 0; // Built-in command nahi hai, external command ke tor par chalayein
}
