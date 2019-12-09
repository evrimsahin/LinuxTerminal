#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_LINE 80 /* The maximum length command */
#define HISTORY_COUNT 10 /* The maximum length of history */

char *history[HISTORY_COUNT];
int history_size = 0;

enum RedirectionType {

    Default = 1,
    StdOutTruncate = 2,
    StdOutAppend = 4,
    StdIn = 8,
    StdErr = 16

};

void execute_command(char **paths, char *command);

void execute_history(char **paths, char *line, int *should_run);

char **split(char *str, char *token);

int count(char **str);

char *trim(char *str);

void add_history(char *str);

void print_history();

void batch_mode(char **path, char *file_name);

void shell_mode(char **paths);

void line_operator(char **paths, char *line, char *result);

void commandSeperator(char *s, char **a);

void path_subtract(char **paths, char *line, char *result);

int redirect(char *file, int redirectionDest, int flags, int mode);

int redirection(enum RedirectionType type, char *inFile, char *outFile);


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
    int i;
    if (history_size == HISTORY_COUNT) {
        for (i = HISTORY_COUNT - 1; i > 0; i--) {
            history[i] = history[i - 1];
        }
        history[0] = temp;
    } else if (history_size > 0) {
        for (i = history_size; i > 0; i--) {
            history[i] = history[i - 1];
        }
        history[0] = temp;
        history_size++;
    } else {
        history[0] = temp;
        history_size++;
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
    } else if (strstr(line, "history -i") != 0) {
        execute_history(paths, line, should_run);
    } else if (strstr(line, "path")) {
        add_history(line);
        if (strstr(line, "+ ")) {
            char *result = NULL;
            result = malloc(strlen(paths) + strlen(line) + 1);
            line_operator(paths, line, result);
            strcpy(paths[0], result);
            putenv(paths[0]);
        } else if (strstr(line, "- ")) {
            char *result = NULL;
            result = malloc(strlen(paths));
            path_subtract(paths, line, result);
            strcpy(paths[0], result);
            putenv(paths[0]);
        } else
            printf("%s\n", getenv("PATH"));
    } else if (strstr(line, ";")) {
        char *a[10];
        int i = 0;
        int count = 0;
        for (i = 0; i < strlen(line); i++) {
            if (line[i] == ';')
                count++;
        }
        commandSeperator(line, a);
        for (int i = 0; i < count + 1; i++) {
            add_history(a[i]);
            execute_command(paths, a[i]);
        }


    } else {
        add_history(line);
        execute_command(paths, line);
    }
}

void path_subtract(char **paths, char *line, char *result) {
    char *token = strtok(line, "- ");

    int k = 0;
    for (k = 0; k < 1; k++) {
        token = strtok(NULL, "- ");
    }


    int i, j, found;
    int strLen, wordLen;
    strLen = strlen(paths[0]);  // Find length of string
    wordLen = strlen(token); // Find length of word


    int x = 0;
    /* Run a loop from starting index of string to length of string - word length */
    for (i = 0; i < strLen - wordLen + 1; i++) {

        // Match word at current position
        found = 1;
        for (j = 0; j < wordLen; j++) {
            // If word is not matched
            if (paths[0][i + j] != token[j]) {
                found = 0;
                break;
            }
        }

        // If word have been found then print found message
        if (found == 1) {
            for (x; x < i; x++) {
                char ch = paths[0][x];

                strncat(result, &ch, 1);
            }
            x = x + wordLen;
        }
    }
    for (x; x < strLen; x++) {
        char ch = paths[0][x];
        strncat(result, &ch, 1);
    }
}

void commandSeperator(char *s, char **a) {


    char *str = strdup(s);
    char *token = strtok(str, ";");
    int j = 0;
    while (token) {

        int count = 0;

        int i;
        for (i = 0; token[i]; i++) {
            if (token[i] != ' ')
                token[count++] = token[i];
        }

        token[count] = '\0';

        a[j] = strdup(token);
        j++;

        int size = strlen(token);

        token = strtok(NULL, ";");
    }

    return;
}

void line_operator(char **paths, char *line, char *result) {

    char *token = strtok(line, "+ ");

    int i = 0;
    for (i = 0; i < 1; i++) {
        token = strtok(NULL, "+ ");
    }
    strcpy(result, paths[0]);
    strcat(result, token);
}

void execute_history(char **paths, char *line, int *should_run) {
    size_t length = strlen(line);
    if (length < 12) {
        printf("Command doesnt include index\n");
    }

    if (length > 12) {
        printf("Command index not greater than 9\n");
    }
    if (line[11] >= '0' && line[11] <= '9' && history_size >= (line[11] - '0')) {
        check_line(paths, history[line[11] - '0'], should_run);
    } else {
        printf("Command not found in history.\n");
    }
}


int check_path(char *path, char **args, int background) {
    if (access(path, 0) != -1) {
        pid_t pid = fork();
        enum RedirectionType redType = Default;
        int i;
        char *inFile = NULL;
        char *outFile = NULL;
        int firstRedirOpPos = INT_MAX;
        if (pid == 0) {
            for (i = 0; args[i] != NULL; i++) {
                if (strcmp(args[i], ">") == 0) {
                    redType |= StdOutTruncate;
                    outFile = args[i + 1];
                    firstRedirOpPos = i < firstRedirOpPos ? i : firstRedirOpPos;
                }
                if (strcmp(args[i], ">>") == 0) {
                    redType |= StdOutAppend;
                    outFile = args[i + 1];
                    firstRedirOpPos = i < firstRedirOpPos ? i : firstRedirOpPos;
                }
                if (strcmp(args[i], "<") == 0) {
                    redType |= StdIn;
                    inFile = args[i + 1];
                    firstRedirOpPos = i < firstRedirOpPos ? i : firstRedirOpPos;
                }
                if (strcmp(args[i], "2>") == 0) {
                    redType |= StdErr;
                    outFile = args[i + 1];
                    firstRedirOpPos = i < firstRedirOpPos ? i : firstRedirOpPos;
                }
            }

            if (redType != Default) {
                printf("inFile: %s\noutFile: %s\n", inFile, outFile);
                args[firstRedirOpPos] = (char *) NULL; // By setting it to NULL, we limit args to arguments priot to it in execv().
                redirection(redType, inFile, outFile);
            }
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

int redirect(char *file, int redirectionDest, int flags, int mode) {
    int fileDesc;

    // 1. Open the file.
    fileDesc = open(file, flags, mode);
    if (fileDesc == -1) {
        fprintf(stderr, "Failed to open the file.");
        return 1;
    }
    // 2. Duplicate the desired descriptor.
    if (dup2(fileDesc, redirectionDest) == -1) {
        switch (redirectionDest) {
            case STDIN_FILENO:
                fprintf(stderr, "Failed to redirect STDIN.");
                break;
            case STDOUT_FILENO:
                fprintf(stderr, "Failed to redirect STDOUT.");
                break;
            case STDERR_FILENO:
                fprintf(stderr, "Failed to redirect STDERR.");
                break;
        }
        return 1;
    }
    // 3. Close the file since we don't need it anymore.
    if (close(fileDesc) == -1) {
        fprintf(stderr, "Failed to close the file.");
        return 1;
    }
}


int redirection(enum RedirectionType type, char *inFile, char *outFile) {
    int writingMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int readingMode = S_IRUSR | S_IRGRP | S_IROTH;
    int result = 0;

    if (type & StdOutTruncate) {
        result = redirect(outFile, STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC, writingMode);
    } else if (type & StdOutAppend) {
        result = redirect(outFile, STDOUT_FILENO, O_WRONLY | O_CREAT | O_APPEND, writingMode);
    }

    if (type & StdIn) {
        result = redirect(inFile, STDIN_FILENO, O_RDONLY, readingMode);
    }

    if (type & StdErr) {
        result = redirect(outFile, STDERR_FILENO, O_WRONLY | O_CREAT | O_APPEND, writingMode);
    }

    return result;
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