#include "shell.h"

// Helper function: Ek single command ke liye '<' aur '>' handle kar ke execvp() chalata hai
void run_command_with_redirection(char** args) {
    char* input_file = NULL;
    char* output_file = NULL;
    char* clean_args[MAXARGS];
    int j = 0;

    // '<' aur '>' operators aur unke filenames ko alag karein
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] != NULL) {
                input_file = args[++i];
            } else {
                fprintf(stderr, "Syntax error: expected file after '<'\n");
                exit(1);
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] != NULL) {
                output_file = args[++i];
            } else {
                fprintf(stderr, "Syntax error: expected file after '>'\n");
                exit(1);
            }
        } else {
            clean_args[j++] = args[i];
        }
    }
    clean_args[j] = NULL;

    // Input Redirection (<)
    if (input_file != NULL) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) {
            perror("open input file");
            exit(1);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    // Output Redirection (>)
    if (output_file != NULL) {
        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("open output file");
            exit(1);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    if (clean_args[0] == NULL) {
        exit(0);
    }

    execvp(clean_args[0], clean_args);
    perror("Command execution failed");
    exit(1);
}

int execute(char* arglist[]) {
    // Check karein ke command mein pipe '|' hai ya nahi
    int pipe_idx = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_idx = i;
            break;
        }
    }

    // CASE 1: Pipe ('|') mojood hai
    if (pipe_idx != -1) {
        arglist[pipe_idx] = NULL; // Left aur right commands ko split karein
        char** left_args = arglist;
        char** right_args = &arglist[pipe_idx + 1];

        if (left_args[0] == NULL || right_args[0] == NULL) {
            fprintf(stderr, "Syntax error near '|'\n");
            return -1;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            return -1;
        }

        // Left Child (Writer)
        pid_t pid1 = fork();
        if (pid1 == 0) {
            close(pipefd[0]);               // Read-end band karein
            dup2(pipefd[1], STDOUT_FILENO); // stdout ko pipe ke write-end par redirect karein
            close(pipefd[1]);
            run_command_with_redirection(left_args);
        }

        // Right Child (Reader)
        pid_t pid2 = fork();
        if (pid2 == 0) {
            close(pipefd[1]);              // Write-end band karein
            dup2(pipefd[0], STDIN_FILENO); // stdin ko pipe ke read-end par redirect karein
            close(pipefd[0]);
            run_command_with_redirection(right_args);
        }

        // Parent Process: Dono pipe ends band karein aur dono children ka wait karein
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return 0;
    }

    // CASE 2: Normal command (ya sirf '<' / '>' redirection)
    int status;
    pid_t cpid = fork();
    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0:
            run_command_with_redirection(arglist);
            break;
        default:
            waitpid(cpid, &status, 0);
            return status;
    }
    return 0;
}
