#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>

int run_single_line(char* line);
void handle_if_block(char* first_line);

#define MAX_JOBS 64

typedef struct {
    pid_t pid;
    char cmd[256];
} Job;

void add_job(pid_t pid, const char* cmd);
void remove_job(pid_t pid);
void reap_zombies();
void print_jobs();

void add_history_cmd(const char* cmd);
void print_history();
char* get_history_cmd(int n);
// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);

#endif // SHELL_H
