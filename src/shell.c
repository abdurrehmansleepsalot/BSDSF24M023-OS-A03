#include "shell.h"
char* history[HISTORY_SIZE];
int history_count = 0;

Job job_list[MAX_JOBS];
int job_count = 0;

void add_job(pid_t pid, const char* cmd) {
    if (job_count < MAX_JOBS) {
        job_list[job_count].pid = pid;
        strncpy(job_list[job_count].cmd, cmd, sizeof(job_list[job_count].cmd) - 1);
        job_list[job_count].cmd[sizeof(job_list[job_count].cmd) - 1] = '\0';
        job_count++;
        printf("[Background PID %d] %s\n", pid, cmd);
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                job_list[j] = job_list[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void reap_zombies() {
    int status;
    pid_t pid;
    // Non-blocking loop: jab tak koi finished child milta rahe, usay reap karein
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

void print_jobs() {
    reap_zombies(); // Print karne se pehle check karein ke koi job khatam to nahi ho gayi
    if (job_count == 0) {
        printf("No active background jobs.\n");
        return;
    }
    for (int i = 0; i < job_count; i++) {
        printf("[%d] PID: %d  %s\n", i + 1, job_list[i].pid, job_list[i].cmd);
    }
}
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
    printf("  cd <dir>      - Change the current working directory\n");
    printf("  exit          - Terminate the shell\n");
    printf("  help          - Display this help message\n");
    printf("  jobs          - List active background jobs\n");
    printf("  history       - Display command history\n");
    printf("  set           - Display all shell variables\n");
    printf("  VAR=value     - Set a shell variable\n");
    return 1;
}

    // 4. jobs command (placeholder)
    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }
    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }
    // 6. set command
    if (strcmp(arglist[0], "set") == 0) {
        print_shell_vars();
        return 1;
    }

    // 7. Variable assignment check: VARNAME=value (bina spaces ke)
    char* eq_ptr = strchr(arglist[0], '=');
    if (eq_ptr != NULL && eq_ptr != arglist[0] && arglist[1] == NULL) {
        *eq_ptr = '\0';
        char* var_name = arglist[0];
        char* var_val = eq_ptr + 1;

        // Agar value ke gird quotes ("...") hon to unhein strip kar dein
        size_t vlen = strlen(var_val);
        if (vlen >= 2 && ((var_val[0] == '"' && var_val[vlen - 1] == '"') ||
            (var_val[0] == '\'' && var_val[vlen - 1] == '\''))) {
            var_val[vlen - 1] = '\0';
        var_val++;
            }

            set_shell_var(var_name, var_val);
            *eq_ptr = '='; // Original string restore karein taake free() mein masla na ho
            return 1;
    }

    return 0; // Built-in command nahi hai, external command ke tor par chalayein
}
// Leading spaces/tabs hatane ke liye chota helper
char* trim_whitespace(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

// Kisi bhi ek command line (including ';' chaining) ko chalane ke liye helper
int run_single_line(char* line) {
    int last_status = 0;
    char* line_copy = strdup(line);
    char* saveptr;
    char* subcmd = strtok_r(line_copy, ";", &saveptr);

    while (subcmd != NULL) {
        char** arglist = tokenize(subcmd);
        if (arglist != NULL) {
            expand_variables(arglist); // Execution se pehle $VAR expand karein
            if (arglist[0] != NULL) {
                if (!handle_builtin(arglist)) {
                    last_status = execute(arglist);
                } else {
                    last_status = 0;
                }
            }
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        subcmd = strtok_r(NULL, ";", &saveptr);
    }

    free(line_copy);
    return last_status;
}

// Multi-line if-then-else-fi block handler
void handle_if_block(char* first_line) {
    char* trimmed = trim_whitespace(first_line);
    // "if " ke baad wali condition command nikalein
    char* cond_cmd = trim_whitespace(trimmed + 2);

    if (strlen(cond_cmd) == 0) {
        fprintf(stderr, "Syntax error: missing condition after 'if'\n");
        return;
    }

    char* then_cmds[64];
    char* else_cmds[64];
    int then_count = 0;
    int else_count = 0;

    // State: 0 = waiting for 'then', 1 = reading 'then' block, 2 = reading 'else' block
    int state = 0;

    while (1) {
        char* next_line = readline("> ");
        if (next_line == NULL) {
            fprintf(stderr, "Syntax error: unexpected EOF while looking for 'fi'\n");
            break;
        }

        char* clean_line = trim_whitespace(next_line);
        if (strlen(clean_line) == 0) {
            free(next_line);
            continue;
        }

        if (strcmp(clean_line, "then") == 0) {
            if (state != 0) {
                fprintf(stderr, "Syntax error: unexpected 'then'\n");
                free(next_line);
                break;
            }
            state = 1;
            free(next_line);
        } else if (strcmp(clean_line, "else") == 0) {
            if (state != 1) {
                fprintf(stderr, "Syntax error: unexpected 'else'\n");
                free(next_line);
                break;
            }
            state = 2;
            free(next_line);
        } else if (strcmp(clean_line, "fi") == 0) {
            free(next_line);
            break; // Block complete!
        } else {
            if (state == 1 && then_count < 64) {
                then_cmds[then_count++] = strdup(clean_line);
            } else if (state == 2 && else_count < 64) {
                else_cmds[else_count++] = strdup(clean_line);
            } else if (state == 0) {
                fprintf(stderr, "Syntax error: expected 'then'\n");
                free(next_line);
                break;
            }
            free(next_line);
        }
    }

    // Condition command execute karein aur exit status check karein
    int raw_status = run_single_line(cond_cmd);
    int exit_code = 1;
    if (WIFEXITED(raw_status)) {
        exit_code = WEXITSTATUS(raw_status);
    }

    // Exit code 0 = Success ('then' block), Non-zero = Failure ('else' block)
    if (exit_code == 0) {
        for (int i = 0; i < then_count; i++) {
            run_single_line(then_cmds[i]);
        }
    } else {
        for (int i = 0; i < else_count; i++) {
            run_single_line(else_cmds[i]);
        }
    }

    // Allocated memory free karein
    for (int i = 0; i < then_count; i++) free(then_cmds[i]);
    for (int i = 0; i < else_count; i++) free(else_cmds[i]);
}


VarNode* var_head = NULL;

// Linked List mein variable add ya update karein
void set_shell_var(const char* name, const char* value) {
    VarNode* curr = var_head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            free(curr->value);
            curr->value = strdup(value);
            return;
        }
        curr = curr->next;
    }

    VarNode* new_node = (VarNode*)malloc(sizeof(VarNode));
    new_node->name = strdup(name);
    new_node->value = strdup(value);
    new_node->next = var_head;
    var_head = new_node;
}

// Variable ki value lookup karein (pehle apni list mein, phir system environment mein)
const char* get_shell_var(const char* name) {
    VarNode* curr = var_head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    char* env_val = getenv(name);
    return (env_val != NULL) ? env_val : "";
}

// 'set' built-in command ke liye saare variables print karein
void print_shell_vars() {
    VarNode* curr = var_head;
    if (curr == NULL) {
        printf("No shell variables set.\n");
        return;
    }
    while (curr != NULL) {
        printf("%s=%s\n", curr->name, curr->value);
        curr = curr->next;
    }
}

// Tokenized arglist mein '$' se shuru hone wale arguments ko expand karein
void expand_variables(char** arglist) {
    if (arglist == NULL) return;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$' && strlen(arglist[i]) > 1) {
            const char* val = get_shell_var(&arglist[i][1]);
            free(arglist[i]); // Purana "$VAR" token free karein
            arglist[i] = strdup(val); // Nayi value allocate karein
        }
    }
}
