#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Builtin shell commands:
 */
int wash_cd(char **args);
int wash_help(char **args);
int wash_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &wash_cd,
    &wash_help,
    &wash_exit
};

int wash_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int wash_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "wash: expect argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("wash: chdir() error in wash_cd");
        }
    }
    return 1;
}

int wash_help(char **args) {
    int i;
    printf("==WASH==\n");
    printf("Builtin Commands:\n");

    for (i = 0; i < wash_num_builtins(); i++) {
        printf(" %s\n", builtin_str[i]);
    }

    printf("========\n");
    return 1;
}

int wash_exit(char **args) {
    return 0;
}

int wash_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("wash: execvp() error in wash_launch");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("wash: fork() error in wash_launch");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int wash_execute(char **args) {
    int i;
    
    if (args[0] == NULL) {
        // empty command
        return 1;
    }

    for (i = 0; i< wash_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return wash_launch(args);
}


char *wash_read_line(void) {
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

#define WASH_TOK_BUFSIZE 64
#define WASH_TOK_DELIM " \t\r\n\a"
char **wash_split_line(char *line) {
    int bufsize = WASH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "wash: alllocation error\n");
        exit(EXIT_FAILURE);
    } 

    token = strtok(line, WASH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += WASH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "wash: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, WASH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void wash_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("|> ");
        line = wash_read_line();
        args = wash_split_line(line);
        status = wash_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    // TODO: Load config files.

    // Run command loop.
    wash_loop();

    // TODO: Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}