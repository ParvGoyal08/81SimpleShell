#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>

#define max_sz_of_input 1000
#define max_sz_of_history 1000

// Declare global variables for command history, process tracking, and execution times

int exec_time = 0;       // Counter for execution times
int history_num = 0;    // Tracks the number of commands in history
int pid_num = 0;    // Counter for process IDs
int start_num = 0;      // Counter for start times 
char *history[max_sz_of_history];   // Stores command history
time_t start_time[max_sz_of_history];   // Stores the start time of each process
pid_t process_pid[max_sz_of_history];   // Stores process IDs
double cpu_time[max_sz_of_history];     // Stores the execution time of each process
// Adds a command to the history, shifting the oldest entry out if the history is full

// Launches a single command with optional background execution
int launch(char *command, char *args[], bool background) {
    // reference taken from https://www.geeksforgeeks.org/time-function-in-c/, https://stackoverflow.com
    time_t start, end;
    int pid, pid_1;
    pid = fork(); // Fork a new process
    
    if (pid < 0) {
        perror("Error while forking");
        return -1;
    } else if (pid == 0) {
            // Child process executes the command

        if (execvp(command, args) == -1) {
            perror("execvp failed to initialize");      // Handle execution failure
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent Process
        if (background) {
            printf("Background Process PID: %d\n", pid);    // Print background process info if background process
            process_pid[pid_num++] = pid;
        } else {
            process_pid[pid_num++] = pid;   // Store foreground process ID
            time(&start);       // Capture start time
            waitpid(pid, NULL, 0);      // Wait for process to complete
            time(&end);     // Capture end time
            cpu_time[exec_time++] = difftime(end, start);       // Record execution time
            start_time[start_num++] = start;    // Record start time
        }
    }
    return pid;
}

void addHistory(char *command) {
    if (history_num < max_sz_of_history) {
        history[history_num++] = strdup(command);   // Add new command to history
    } else {
                // If history is full, free the oldest command and shift the rest
        free(history[0]);
        for (int i = 0; i < max_sz_of_history - 1; i++) {
            history[i] = history[i + 1];
        }
        history[max_sz_of_history - 1] = strdup(command);   // Add new command at the end
    }
}

bool parsing(char *input, int *counter, char *commands[]) {
    *counter = 0;
    char *token = strtok(input, " ");
    bool background = false;
    // reference taken from https://www.geeksforgeeks.org/how-to-split-a-string-in-cc-python-and-java/

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            background = true;
        } else {
            commands[(*counter)++] = token;
        }
        token = strtok(NULL, " ");
    }
    commands[*counter] = NULL;  // Null-terminate the commands array
    return background;
}


int create_pipe(char *commands[][max_sz_of_input], int command_count) {
    // referance taken from https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c
    int fd[2]; // File descriptor array for the pipe
    int pid_prev = -1; // Process ID of the previous command
    time_t start, end;

    for (int i = 0; i < command_count; i++) {
        // Create a new pipe for each command (except the last one)
        if (i < command_count - 1) {
            if (pipe(fd) < 0) {
                perror("Error: Pipe creation failed");
                return -1;
            }
        }
        // Fork the current command
        int pid = fork();
        if (pid < 0) {
            perror("Error: Fork failed");
            return -1;
        } else if (pid == 0) {
            // In child process
            if (pid_prev != -1) {
                dup2(pid_prev, STDIN_FILENO); // Redirect stdin from previous command
                close(pid_prev);
            }
            if (i < command_count - 1) {
                dup2(fd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
                close(fd[0]);
                close(fd[1]);
            }
            if (execvp(commands[i][0], commands[i]) < 0) {
                perror("Error: Failed to execute command");
                exit(EXIT_FAILURE);
            }
        } else {
            // In parent process
            if (pid_prev != -1) {
                close(pid_prev);
            }
            if (i < command_count - 1) {
                close(fd[1]); // Close the write end in the parent
                pid_prev = fd[0]; // Save the read end for the next command
            }
            // Wait for the current command to finish
            waitpid(pid, NULL, 0);
        }
    }
    
    // Record execution time
    time(&start);
    time(&end);
    cpu_time[exec_time++] = difftime(end, start);
    start_time[start_num++] = start;

    return 0;
}


void sigchld_handler(int signo) {
    (void) signo; // Unused parameter
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Reap all terminated child processes
    }
}

void process_command(char *input) {
    int counter;
    int pipe_num = 0;
    char *commands[max_sz_of_input];

    // Check for pipes in the input
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '|') {
            pipe_num++;
        }
    }

    input[strcspn(input, "\n")] = '\0';  // Remove trailing newline
    addHistory(input);
    bool background = parsing(input, &counter, commands);
    commands[counter] = NULL;  // Null-terminate the commands array

    if (pipe_num == 0) {
        launch(commands[0], commands, background);  // If no pipe, execute the command
    } else {
        // Handle multiple pipes
        char *pipe_commands[max_sz_of_input][max_sz_of_input]; // 2D array for commands
        int command_count = 0;  // Number of commands
        int part = 0;           // Command part index

        // Extract commands split by pipes
        for (int i = 0; i <= counter; i++) {
            if (commands[i] != NULL && strcmp(commands[i], "|") != 0) {
                pipe_commands[command_count][part++] = commands[i];
            } else {
                if (part > 0) {
                    pipe_commands[command_count][part] = NULL; // Null-terminate the command
                    command_count++;
                    part = 0; // Reset part for the next command
                }
            }
        }

        // Call create_pipe with all extracted commands
        create_pipe(pipe_commands, command_count);
    }
}


void shell_loop() {
    // Set up the SIGCHLD handler for handling child process termination
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;  // Restart interrupted system calls, do not receive signals for stopped children
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    char input[max_sz_of_input];  // Input buffer for command

    while (true) {
        // Display shell prompt
        printf("dhruv@DESKTOP-4FHN094:~$  ");
        fflush(stdout);  // Ensure the prompt is displayed immediately

    if (fgets(input, sizeof(input), stdin) == NULL || strcmp(input, "exit\n") == 0) {
        // Handle EOF or exit command, print history and exit
        for (int i = 0; i < history_num; i++) {
            printf("%d: %s\n", i + 1, history[i]);
            printf("PID: %d\n", process_pid[i]);
            printf("Execution time: %.2f s\n", cpu_time[i]);
            printf("Start time: %s", ctime(&start_time[i]));
        }
        break; // Exit the loop
    }


        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';  


        if (strcmp(input, "history") == 0) {
            // Handle history command
            for (int i = 0; i < history_num; i++) {
                printf("%d: %s\n", i + 1, history[i]);
            }
        } else if (strncmp(input, "./", 2) == 0 && strstr(input, ".sh") != NULL) {
            // If input is a script, execute the script line by line
            FILE *file = fopen(input, "r");
            if (file == NULL) {
                perror("Error reading script file");
                continue;
            }

            while (fgets(input, sizeof(input), file) != NULL) {
                if (strstr(input, "#!") != NULL) {
                    continue;  // Skip script header
                }
                process_command(input);  // Process each line as a command
            }
            fclose(file);
        } else {
            process_command(input);  // Process normal input
        }
    }
}


int main() {
    shell_loop();
    // Free memory allocated for history before exiting
    for (int i = 0; i < history_num; i++) {
        free(history[i]);
    }
    return 0;
}
