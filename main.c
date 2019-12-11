#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_LINE 80 /* The maximum length of a command */
#define HISTORY_COUNT 10 /* The maximum length of history*/

char *history[HISTORY_COUNT]; //History array
int history_size = 0; //Number of added commands in the history list
int myPathSize = 0;
char *mypaths[100];
int mypids[20];    // this array for background pids.
int pid_index = 0; //The number �f background processes


void execute_command(char **paths, char *command);

void execute_history(char **paths, char *line, int *should_run);

char **split(char *str, char *token);

int count(char **str);

char *trim(char *str);

void add_history(char *str);

void print_history();

void shell_mode(char **paths);

void line_operator(char **paths, char *line);

void command_seperator(char *s, char **a);

void path_subtract(char **paths, char *line);

int redirect(char *file, int redirectionDest, int flags, int mode);

int redirection(int type, char *inFile, char *outFile);

void check_line(char **paths, char *line, int *should_run);

void backToForeground(char *line);

int main(int argc, char *argv[]) {
    char **path = split(getenv("PATH"), ":");
    shell_mode(path);
    return 0;
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
    if (history_size == HISTORY_COUNT) { //If the array is not empty, adds to the head, last element is removed.
        for (i = HISTORY_COUNT - 1; i > 0; i--) {
            history[i] = history[i - 1];
        }
        history[0] = temp;
    }
    else if (history_size > 0) { //If the array is not full
        for (i = history_size; i > 0; i--) { //Shifts elements by one
            history[i] = history[i - 1];
        }
        history[0] = temp; //Adds to the head
        history_size++;
    }
    else { //If the array is empty
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

void check_line(char **paths, char *line, int *should_run) { //Checks for possible command cases
    line = trim(line); //Takes the first word as input(command), removes leading and trailing blanks

    if (strlen(line) == 0) { //If no input is entered.
        return;
    }
    if (strcmp(line, "exit") == 0) { //If command is "exit", makes should_run 0, which means shell won't work again.
        *should_run = 0;
    }
    else if (strcmp(line, "history") == 0) { //If the command includes "history"
        add_history(line); //Adds the command to the history array
        print_history(); //Prints the history array
    }
    else if (strstr(line, "history -i") != 0) { //If the command includes "history -i"
        execute_history(paths, line, should_run);
    }
    else if (strstr(line, "path")) { //Cases for path command
        add_history(line);

        if (strstr(line, "+ ")) { //If the command includes "+"
            line_operator(mypaths, line);
            myPathSize++;
        }
        else if (strstr(line, "- ")) { //If the path command includes "-"
            path_subtract(mypaths, line);
        }
        else //If the command is just "path"
            for(int i = 0; i<myPathSize;i++) { //Prints our path array.
                if(mypaths[i] != 0)
                    printf("%s\n", mypaths[i]);
            }
    }
    else if (strstr(line, ";")) { //If the command includes ";", which means multiple commands
        char *commandList[10]; //2d command array. Multiple commands will be stored in here
        int i = 0;
        int count = 0;
        for (i = 0; i < strlen(line); i++) { //Number of commands will be equal to count + 1
            if (line[i] == ';')
                count++;
        }
        command_seperator(line, commandList); //Seperates multiple commands and store them in a 2d commandList array
        for (int i = 0; i < count + 1; i++) {
            add_history(commandList[i]); //Adds these commands to the history
            execute_command(paths, commandList[i]); //Executes the commands in order
        }
    }
    else if (strstr(line, "fg")) { //If the command includes "fg"
        add_history(line); //Adds the command to the history
        backToForeground(line); //Moves the background process to the foreground
    }

    else { //If the command is not one of the possible commands listed above
        add_history(line); //Adds the command in the history array
        execute_command(paths, line);
    }
}

void backToForeground(char *line) {
    char *token = strtok(line, "%");
    int id;
    int k = 0;
    for (k = 0; k < 1; k++) { //Gets the id of the background process
        token = strtok(NULL, "%");
    }
    id = atoi(token);  //convert string to integer
    int status = 0;

    for (int i = 0; i < pid_index; ++i) {
        if (id == mypids[i]) { //If the id is inside of the array of the background processes
            kill(id, SIGCONT);     //move the background proccess to foreground proccess
            waitpid(id, &status, WUNTRACED);
        }
    }
    pid_index--;
}

void path_subtract(char **mynew, char *line) {

    char *token = strtok(line, "- "); //Returns the first token. "path" is expected as first token.

    int k = 0;
    for (k = 0; k < 1; k++) {
        token = strtok(NULL, "- "); //Returns the second token. We expect a new path in here.
    }

    int t=0;
    for(t = 0; t < myPathSize; t++){ //Iterates through the path array.
        char mystr[strlen(mynew[t])]; //Temporary char array named as mystr is created,
        strcpy(mystr, mynew[t]); //and path array's t-th element is stored inside it.

        if(strcmp(mystr, token) == 0){ //If the input path is inside the path array, makes in null(remove).
            mynew[t] = NULL;
        }
    }
}

void command_seperator(char *s, char **a) {

    char *str = strdup(s); //Full line, for example "ls; pwd".
    char *token = strtok(str, ";"); //Returns the first token
    int j = 0;

    while (token) { //If the token is not null

        int count = 0;

        //This part removes blanks in the token, if there exists
        int i;
        for (i = 0; token[i]; i++) {
            if (token[i] != ' ')
                token[count++] = token[i];
        }
        token[count] = '\0';

        a[j] = strdup(token); //Stores the token(command) in the a(command list)

        j++;

        token = strtok(NULL, ";"); //Next token(next command after ; -if there is-
    }
    return;
}

void line_operator(char **paths, char *line) {

    char *token = strtok(line, "+ ");

    int i = 0;
    for (i = 0; i < 1; i++) {
        token = strtok(NULL, "+ ");
    }
    paths[myPathSize]=strdup(token);
}

void execute_history(char **paths, char *line, int *should_run) {
    size_t length = strlen(line); //Length of command line
    //History index must be between 0 and 9(included)
    if (length < 12) {
        printf("Command doesnt include index\n");
    }
    if (length > 12) {
        printf("Command index not greater than 9\n");
    }
    if (line[11] >= '0' && line[11] <= '9' && history_size >= (line[11] - '0')) {  //If the given index is in history
        check_line(paths, history[line[11] - '0'], should_run);
    }
    else {
        printf("Command not found in history.\n");
    }
}


int check_path(char *path, char **args, int background) {
    if (access(path, 0) != -1) { //If the path is accessible
        pid_t pid = fork(); //Forks a child process
        int redType = 0; //Initially zero
        int i;
        char *inputFile = NULL; //Input file
        char *outputFile = NULL; //Output file
        int option = INT_MAX;
        if (pid == 0) { //Child process
            for (i = 0; args[i] != NULL; i++) {
                if (strcmp(args[i], ">") == 0) {
                    redType = 1;
                    outputFile = args[i + 1];
                    option = i < option ? i : option;
                }
                if (strcmp(args[i], ">>") == 0) {
                    redType = 2;
                    outputFile = args[i + 1];
                    option = i < option ? i : option;
                }
                if (strcmp(args[i], "<") == 0) {
                    redType = 3;
                    inputFile = args[i + 1];
                    option = i < option ? i : option;
                }
                if (strcmp(args[i], "2>") == 0) {
                    redType = 4;
                    outputFile = args[i + 1];
                    option = i < option ? i : option;
                }
            }

            if (redType != 0) {
                printf("inputFile: %s\noutputFile: %s\n", inputFile, outputFile);
                args[option] = (char *) NULL;
                redirection(redType, inputFile, outputFile);
            }
            execv(path, args); //Executes the given command
            //exit(0);
        }
        else {
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                mypids[pid_index] = pid;
                pid_index++;
                printf("PID : %d\n", pid);

            }
            return 1;
        }
    }
    return 0;
}

int redirect(char *file, int redirectionDest, int flags, int mode) {
    int fileDesc;

    // Opens the file.
    fileDesc = open(file, flags, mode);
    if (fileDesc == -1) {
        fprintf(stderr, "Failed to open the file.");
        return 1;
    }
    if (dup2(fileDesc, redirectionDest) == -1) {
        switch (redirectionDest) {
            case STDIN_FILENO:
                fprintf(stderr, "Failed STDIN.");
                break;
            case STDOUT_FILENO:
                fprintf(stderr, "Failed STDOUT.");
                break;
            case STDERR_FILENO:
                fprintf(stderr, "Failed STDERR.");
                break;
        }
        return 1;
    }
    // Closes the file since we don't need it anymore.
    if (close(fileDesc) == -1) {
        fprintf(stderr, "Failed to close the file.");
        return 1;
    }
}


int redirection(int type, char *inFile, char *outFile) {
    int writingMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int readingMode = S_IRUSR | S_IRGRP | S_IROTH;
    int result = 0;

    //Based on the redirection type, chooses the option and updates the result
    if (type == 1) {
        result = redirect(outFile, STDOUT_FILENO, O_WRONLY | O_CREAT | O_TRUNC, writingMode);
    } else if (type == 2) {
        result = redirect(outFile, STDOUT_FILENO, O_WRONLY | O_CREAT | O_APPEND, writingMode);
    }

    if (type == 3) {
        result = redirect(inFile, STDIN_FILENO, O_RDONLY, readingMode);
    }

    if (type == 4) {
        result = redirect(outFile, STDERR_FILENO, O_WRONLY | O_CREAT | O_APPEND, writingMode);
    }

    return result;
}


void execute_command(char **paths, char *command) {
    int background = 0; //background or foreground, like boolean

    //Checks whether command includes & or not
    if (command[strlen(command) - 1] == '&') {
        background = 1;
        command[strlen(command) - 1] = '\0';
    }

    char **args = split(command, " ");

    int args_length = count(args);
    args[args_length] = NULL;

    int path_is_good = check_path(args[0], args, background);

    int i;
    for (i = 0; paths[i] != NULL && !path_is_good; i++) { //Traverses all of the paths and tries to find the path it can be executed.
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
    int should_run = 1; //is program still running?
    while (should_run) {
        printf("myshell: ");
        fflush(NULL); //Makes buffer empty

        char line[MAX_LINE]; //Command input
        if (fgets(line, MAX_LINE, stdin) == NULL) { //Reads the command input
            exit(0);
        }
        if (line == NULL) {
            exit(1);
        }
        check_line(paths, line, &should_run);
    }
}
