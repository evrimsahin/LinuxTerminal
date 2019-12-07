#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 80 /* The maximum length command */
#define HISTORY_COUNT 10 /* The maximum length of history */

char *history[HISTORY_COUNT];
int history_size = 0;

void execute_command(char **paths, char *command);

void execute_history(char **paths, char *line, int *should_run);

char **split(char *str, char *token);

int count(char **str);

char *trim(char *str);

void add_history(char *str);

void print_history();

void batch_mode(char **path, char *file_name);

void shell_mode(char **paths);

int main(void) {
    char inputBuffer[MAX_LINE];
    char **paths = split(getenv("PATH"), ":");
    char *args[MAX_LINE / 2 + 1];

    if (args == 2) {
        printf("%s\n", inputBuffer);
        batch_mode(paths, inputBuffer);
    } else {
        shell_mode(paths);
    }
    return 0;
}

char *trim(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char) *str)) {
        str++;
    }

    if (*str == 0) {
        return str;
    }

    // Trim traling space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) {
        end--;
    }

    // Write new NULL terminator
    *(end + 1) = 0;

    return str;
}


int count(char **str) {
    int cnt = 0, i;
    for (i = 0; str[i] != NULL; i++) {
        cnt++;
    }
    return cnt;
}

void add_history(char *str) {
    char *temp = malloc(100);
    strcpy(temp, str);
    if (history_size == HISTORY_COUNT) {
        int i;
        for (i = 0; i < history_size - 1; i++) {
            history[i] = history[i + 1];
        }
        history[history_size - 1] = temp;
    } else {
        history[history_size++] = temp;
    }
}

void print_history() {
    if (history_size == 0) {
        printf("History Empty.\n");
    }
    int i = 0;
    for (i = 0; i < history_size; i++) {
        printf("%d  %s\n", i, history[i]);
    }
}


void check_line(char **paths, char *line, int *should_run) {
    line = trim(line);

    if (strlen(line) == 0) {
        return;
    }

    if (strcmp(line, "exit") == 0) {
        *should_run = 0;
    } else if (strcmp(line, "history") == 0) {
        add_history(line);
        print_history();
    } else if (line[0] == '!') {
        execute_history(paths, line, should_run);
    } else {
        add_history(line);
        execute_command(paths, line);
    }
}

void execute_history(char **paths, char *line, int *should_run) {
    size_t length = strlen(line);
    if (length == 2 && line[1] == '!' && history_size >= 1) {
        check_line(paths, history[history_size - 1], should_run);
    } else if (length == 2 && line[1] > '0' && line[1] <= '9' && history_size >= (line[1] - '0')) {
        check_line(paths, history[line[1] - '0' - 1], should_run);
    } else if (strcmp(line, "!10") == 0 && history_size == 10) {
        check_line(paths, history[9], should_run);
    } else {
        printf("Command not found in history.\n");
    }
}


int check_path(char *path, char **args, int background) {
    if (access(path, 0) != -1) {
        pid_t pid = fork();
        if (pid == 0) {
            execv(path, args);
            exit(0);
        } else {
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
            }
            return 1;
        }
    }
    return 0;
}


void execute_command(char **paths, char *command) {
    int background = 0;

    if (command[strlen(command) - 1] == '&') {
        background = 1;
        command[strlen(command) - 1] = '\0';
    }

    char **args = split(command, " ");

    int args_length = count(args);
    args[args_length] = NULL;

    int path_is_good = check_path(args[0], args, background);

    int i;
    for (i = 0; paths[i] != NULL && !path_is_good; i++) {
        char *path = malloc(sizeof(char) * (strlen(paths[i]) + strlen(args[0]) + 1));
        strcpy(path, paths[i]);
        strcat(path, "/");
        strcat(path, args[0]);
        path_is_good = check_path(path, args, background);
    }

    if (!path_is_good) {
        printf("Incorrect Command.\n");
    }


}


void shell_mode(char **paths) {
    int should_run = 1;
    while (should_run) {
        printf("myshell: ");
        fflush(NULL);

        char line[MAX_LINE];
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            exit(0);
        }
        if (line == NULL) {
            exit(1);
        }
        check_line(paths, line, &should_run);
    }
}


void batch_mode(char **path, char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("The batch file does not exist");
    } else {
        int should_run = 1;
        char command[MAX_LINE];
        while (should_run && fgets(command, MAX_LINE, file) != NULL) {
            printf("%s", command);
            check_line(path, command, &should_run);
        }
        if (should_run) {
            printf("File should end with exit command");
        }
    }
}

char **split(char *str, char *token) {

    char **res = NULL;
    char *temp = strtok(str, token);
    int n_spaces = 0;
    while (temp) {
        res = realloc(res, sizeof(char *) * ++n_spaces);

        if (res == NULL) {
            exit(-1);
        }
        res[n_spaces - 1] = temp;
        temp = strtok(NULL, token);
    }
    res = realloc(res, sizeof(char *) * (n_spaces + 1));
    res[n_spaces] = 0;
    return res;


}