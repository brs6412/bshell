#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>

#define BUF_SIZE 1024
#define MAX_ARGS 10
#define MAX_LEN 128

void execute_cmd(const char *cmd, char **argv) {
    pid_t wpid, pid;
    pid = fork();
    int status;
    if (pid == 0) {
        if (execvp(cmd, argv) == -1) {
            perror("execvp");
        }
        exit(1);
    } else if (pid < 0) {
        perror("fork");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

int is_builtin(const char *cmd) {
    const char *list[] = { "echo", "exit", "type", "pwd", "cd" };
    int found = 0;

    for (size_t i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
        if (strcmp(list[i], cmd) == 0) {
            found = 1;
            break;
        }
    }
    return found;
}

char* is_in_path(const char *cmd) {
    const char *path_env = getenv("PATH");
    char *result = NULL;
    if (!path_env) {
        return result;
    }

    char path_copy[BUF_SIZE];
    strncpy(path_copy, path_env, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *tok = strtok(path_copy, ":");
    while (tok != NULL) {
        char fpath[BUF_SIZE];
        snprintf(fpath, sizeof(fpath), "%s/%s", tok, cmd);
        if (access(fpath, X_OK) == 0) {
            result = malloc(strlen(fpath) + 1);
            if (result) strcpy(result, fpath);
            return result;
        }

        tok = strtok(NULL, ":");
    }
    return result;
}

int main(void) {
    setbuf(stdout, NULL);
    char *fpath, *typepath;
    char input[BUF_SIZE];
    char arg[MAX_LEN];
    char *cmd_args[MAX_ARGS + 1];
    int argc, arg_len;
    bool in_quote, in_dblquote, in_path;
    size_t path_len;

    while (1) {
        printf("$ ");
        fgets(input, BUF_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';
        argc = 0, arg_len = 0;
        char *car = input, *next;
        in_quote = false, in_dblquote = false;

        // Skip any leading whitespace 
        while (*car == ' ') car++;
        while (*car != '\0' && argc < MAX_ARGS) {
            if (*car == '\\') {
                next = car + 1;
                if (*next == '\"' || *next == '\\') {
                    car++;
                    arg[arg_len++] = *car;
                    car++;
                    continue;
                }
            }
            if (in_quote || in_dblquote) {
                if (*car == '\'' && in_quote) {
                    next = car + 1;
                    if (*next == '\'') {
                        car += 2;
                        continue;
                    } else {
                        arg[arg_len] = '\0';
                    }
                    in_quote = false;
                    car++;
                } else if(*car == '\"' && in_dblquote) {
                    next = car + 1;
                    if (*next == '\"') {
                        car += 2;
                        continue;
                    } else if (*next != ' ') {
                        car++;
                        in_dblquote = false;
                        continue;
                    } else {
                        arg[arg_len] = '\0';
                    }
                    in_dblquote = false;
                    car++;
                } else {
                    arg[arg_len++] = *car;
                    car++;
                    continue;
                }
            } else {
                if (*car == ' ') {
                    arg[arg_len] = '\0';
                    car++;
                } else if (*car == '\'' && !in_dblquote) {
                    next = car + 1;
                    if (*next == '\'') {
                        car += 2;
                        continue;
                    } 
                    in_quote = true;
                    car++;
                    continue;
                } else if (*car == '\"') {
                    next = car + 1;
                    if (*next == '\"') {
                        car += 2;
                        continue;
                    }
                    in_dblquote = true;
                    car++;
                    continue;
                } else {
                    arg[arg_len++] = *car;
                    car++;
                    continue;
                }
            }
            cmd_args[argc] = malloc(arg_len + 1);
            if (cmd_args[argc] == NULL) {
                perror("malloc");
                exit(1);
            }
            strcpy(cmd_args[argc++], arg); 
            arg_len = 0;
            while (*car == ' ') car++;
        }
        if (arg_len != 0) {
            arg[arg_len] = '\0';
            cmd_args[argc] = malloc(arg_len + 1);
            if (cmd_args[argc] == NULL) {
                perror("malloc");
                exit(1);
            }
            strcpy(cmd_args[argc++], arg);
        }

        cmd_args[argc] = NULL;

        fpath = is_in_path(cmd_args[0]);
        in_path = fpath && !is_builtin(cmd_args[0]);
        if (strcmp(cmd_args[0], "exit") == 0) {
            break;
        } else if (strcmp(cmd_args[0], "echo") == 0) {
            for (int i = 1; i < argc; i++) {
                if (i == argc - 1) {
                    printf("%s\n", cmd_args[i]);
                } else {
                    printf("%s ", cmd_args[i]);
                }
            }
        } else if (strcmp(cmd_args[0], "type") == 0) {    
            if (cmd_args[1]) {
                if (is_builtin(cmd_args[1])) {
                    printf("%s is a shell builtin\n", cmd_args[1]);
                } else {
                    typepath = is_in_path(cmd_args[1]);
                    if (typepath) {
                        printf("%s is %s\n", cmd_args[1], typepath);
                        free(typepath);
                    } else {
                        printf("%s: not found\n", cmd_args[1]);
                    }
                }
            } else {
                printf("type: missing argument\n");
            }
        } else if (strcmp(cmd_args[0], "pwd") == 0) {
            char cwd[100];
            printf("%s\n", getcwd(cwd, sizeof(cwd)));
        } else if (strcmp(cmd_args[0], "cd") == 0) {
            if (cmd_args[1]) {
                errno = 0;
                char dir[100];
                if (cmd_args[1][0] == '~') {
                    char *home = getenv("HOME");
                    snprintf(dir, sizeof(dir), "%s%s", home, cmd_args[1] + 1);
                } else {
                    strcpy(dir, cmd_args[1]);
                } 
                if (chdir(dir) < 0) {
                    if (errno == ENOENT) {
                        printf("cd: %s: No such file or directory\n", dir);
                    } else {
                        perror("cd"); 
                    }
                }
            } else {
                printf("cd: missing argument\n");
            }
        } else if (in_path) {
            execute_cmd(fpath, cmd_args); 
        } else {
            printf("%s: command not found\n", cmd_args[0]);
        }
        if (fpath) free(fpath);
        for (int i = 0; i < argc; i++) {
            if (cmd_args[i]) free(cmd_args[i]);
        }
        memset(input, 0, sizeof(input));
    }

    return 0;
}
