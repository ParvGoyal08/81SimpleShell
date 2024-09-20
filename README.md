Contributors: 
- Parv Goyal(2023374)
- Dhruv Kumar (2023202)
- Github Repo Link: https://github.com/ParvGoyal08/81SimpleShell/blob/main/README.md



Features:
- Execute single commands.
- Handle commands with pipes.
- Read commands from both standard input and files.
- Exit function ends the Simple Shell implementation
- After Exit, PID, start time and execution time of each command is printed on terminal
- Support commands with piping.
- Maintain a history of executed commands.
- Display process ID (PID), start time, and execution time for each command.



Global Variables:
1. exec_time: Tracks the number of executed commands (used to store execution times).
2. history_num: Tracks the number of commands in the history.
3. pid_num: Tracks the number of process IDs stored.
4. start_num: Tracks the number of recorded start times.
5. history: An array to store command history (char history[max_sz_of_history]).
6. start_time: An array to store the start time of each process (time_t start_time[max_sz_of_history]).
7. process_pid: An array to store process IDs (pid_t process_pid[max_sz_of_history]).
8. cpu_time: An array to store the execution time of each process (double cpu_time[max_sz_of_history]).



Function Details:
1. launch: This function forks a new process to execute a command. If the command is to run in the background, it immediately returns control to the shell; otherwise, it waits for the process to finish and records its execution time and process ID.

2. addHistory: It stores each command the user enters in a history array. If the history is full, it removes the oldest command to make space for new ones.

3. parsing: This function splits the user's input into individual words (tokens) and checks if the command should run in the background by looking for the & symbol.

4. create_pipe: It handles the execution of multiple commands connected via pipes. For each command in a pipe sequence, it creates the necessary pipes and connects the input and output streams of the commands.

5. sigchld_handler: This function handles the termination of background processes by cleaning up "zombie" processes that have finished but haven't been reaped by the parent process.

6. process_command: It analyzes the input to determine if the command contains pipes. If no pipes are present, it runs the command normally; otherwise, it splits the input into multiple commands and calls create_pipe.

7. shell_loop: This is the main loop of the shell. It continuously prompts the user for input, handles built-in commands like history, and processes normal commands or scripts.

8. main: It starts the shell loop, which runs until the user exits. Before the program terminates, it frees the memory allocated for storing the command history.



Limitations:
The simple shell will not work for the cd command, which allows one to change their directory. This is because the cd command changes the directory in the child process, but this change isn't reflected in the main process. In a typical shell, changing directories needs to affect the parent shell, but since our shell forks a new process to run commands, the change only happens in the child process. Thus, the change doesn't persist in our shell session, and when the child process ends, the parent shell remains in the same directory.

Similarly, it doesn't work for commands that modify environment variables because the child process has its own environment. Changes to environment variables made within a child process (export) won't affect the parent process. Therefore, environment variables set or modified in a child process are not inherited by the parent shell. For example, commands like echo $HOME might not work properly if the shell does not handle environment variable expansion, and changes to variables such as PATH or custom variables using export won't persist after the command ends.
