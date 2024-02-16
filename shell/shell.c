#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

char user_input[1024];
char cwd[1024];
char buffer[128];

int current_pipe = 0;

typedef struct process
{
    char *command;
    char *input_file;
    char *output_file;
    char **args;
    int redirect_in;
    int redirect_out;
} process;

void emptyCharArray(char array[])
{
    memset(array, '\0', 1024 * sizeof(char));
}

char *remove_newline(char *str)
{
    int length = strlen(str);

    if (length > 0 && str[length - 1] == '\n')
    {
        str[length - 1] = '\0';
    }

    return str;
}

char *remove_space(char *str)
{
    int length = strlen(str);

    if (length > 0 && str[length - 1] == ' ')
    {
        str[length - 1] = '\0';
    }

    return str;
}

void countArgs(char *input, int *num)
{
    int i;

    *num = 1;

    for (i = 0; i < strlen(input); i++)
    {
        if (input[i] == ' ')
        {
            *num = *num + 1;
        }
    }
}

void countPipes(char *input, int *pipes)
{
    int i;

    for (i = 0; i < strlen(input); i++)
    {
        if (input[i] == '|')
        {
            *pipes = *pipes + 1;
        }
    }
}

void redirect_in(process *process)
{

    int input_fd = open(process->input_file, O_RDONLY);
    if (input_fd == -1)
    {
        perror("open input file");
        exit(EXIT_FAILURE);
    }
    dup2(input_fd, STDIN_FILENO);
    close(input_fd);

    // Read from stdin(file) and put in args

    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        char *ptr = remove_newline(buffer);

        int i = 0;

        while (1)
        {
            if (process->args[i] != NULL)
            {
                i++;
            }
            else
            {
                break;
            }
        }
        int j = 0;

        int allocated_mem = 0;

        while (*ptr)
        {
            if (*ptr == ' ')
            {
                ptr++;
                i++;
                j = 0;
                allocated_mem = 0;
            }
            else
            {

                if (allocated_mem == 0)
                {
                    process->args[i] = malloc(sizeof(char) * 64);
                    allocated_mem = 1;
                }
                else
                {
                    process->args[i][j] = *ptr;
                    j++;
                    ptr++;
                }
            }
        }

        emptyCharArray(buffer);
    }
}

void redirect_out(process *process)
{

    int output_fd = open(process->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1)
    {
        perror("open output file");
        exit(EXIT_FAILURE);
    }
    dup2(output_fd, STDOUT_FILENO);
    close(output_fd);
}

void setup_process(process *process, char *input)
{
    int i = 0;

    char *input_tkn = strdup(input);

    char *p = strtok(input_tkn, " ");

    if (input_tkn[0] == '.')
    {
        input_tkn[0] = ' ';
    }

    if (input_tkn[1] == '/' || input_tkn[0] == '/')
    {
        process->command = "exec";
    }
    else
    {
        strcpy(process->command, remove_newline(p));
    }

    while (p != NULL)
    {

        if (strcmp(p, "<") == 0)
        {
            process->redirect_in++;
            p = strtok(NULL, " ");
            process->input_file = remove_newline(p);
            p = strtok(NULL, " ");
        }
        else if (strcmp(p, ">") == 0)
        {
            process->redirect_out++;
            p = strtok(NULL, " ");
            process->output_file = remove_newline(p);
            p = strtok(NULL, " ");
        }
        else
        {
            process->args[i] = malloc(sizeof(char) * 64);
            process->args[i] = remove_newline(p);
            i++;
        }

        p = strtok(NULL, " ");
    }
}

void exec_cmd_wpipes(process **process, int *pipes, int *current_process)
{
    pid_t pid;
    int status;

    int num_processes = *pipes + 1;

    int pipefd[*pipes][2];
    
    for (int i = 0; i < *pipes; i++)
    {
        if (pipe(pipefd[i]) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Execute commands
    for (int i = 0; i < num_processes; i++)
    {

        pid = fork();

        if (pid == -1)
        {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        { 
            // Not first process
            if (i != 0)
            {
                close(pipefd[i - 1][1]); // Close write of previous pipe              
                dup2(pipefd[i - 1][0], STDIN_FILENO); // Redirect stdin from previous pipe
            }
            // Not last process
            if (i != *pipes)
            {
                close(pipefd[i][0]); // Close read     
                dup2(pipefd[i][1], STDOUT_FILENO);  // Redirect stdout to current pipe write
            }

            // Close all pipe ends
            for (int j = 0; j < *pipes; j++)
            {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            // Handle redirection
            if (process[i]->redirect_in != 0)
            {
                redirect_in(process[i]);
            }
            else if (process[i]->redirect_out != 0)
            {
                redirect_out(process[i]);
            }

            // Execute command
            execvp(process[i]->args[0], process[i]->args);

            // If execvp returns, an error occurred
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipe ends in parent process
    for (int i = 0; i < *pipes; i++)
    {
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_processes; i++)
    {
        wait(&status);
    }
}

void setup_process_wpipes(process **process, char *user_input, int *pipes)
{

    int num_of_processes = *pipes + 1;

    // Keep track of which process is executing
    int *current_process = malloc(sizeof(int));
    *current_process = 0;

    // int *argnum = malloc(sizeof(int));

    char *saveptr;

    char *user_input_cpy = strdup(user_input);

    char *input = strtok_r(user_input_cpy, "|", &saveptr);

    // Setup each individual process
    for (size_t i = 0; i < num_of_processes; i++)
    {
        process[i] = malloc(sizeof(struct process));
        process[i]->command = malloc(sizeof(char) * 64);
        process[i]->redirect_in = 0;
        process[i]->redirect_out = 0;
        process[i]->input_file = malloc(sizeof(char) * 64);
        process[i]->output_file = malloc(sizeof(char) * 64);

        process[i]->args = malloc(sizeof(char *) * 128);

        setup_process(process[i], input);

        input = strtok_r(NULL, "|", &saveptr);
    }

    exec_cmd_wpipes(process, pipes, current_process);
}

void exec_cmd(process *process)
{

    int status;
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {

        if (process->redirect_in != 0)
        {
            redirect_in(process);
        }
        else if (process->redirect_out != 0)
        {
            redirect_out(process);
        }

        int result = execvp(process->args[0], process->args);

        if (result == -1)
        {
            perror("execvp");
            printf("Unrecognized command: %s \n", process->command);

            exit(EXIT_FAILURE);
        }
    }
    else
    {
        pid_t wait_pid = waitpid(pid, &status, 0);

        if (wait_pid == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char const *argv[])
{

    while (1)
    {
        int *argnum = malloc(sizeof(int));
        int *pipes = malloc(sizeof(int));

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);
        fflush(stdout);

        fgets(user_input, sizeof(user_input), stdin);

        countPipes(user_input, pipes);

        process **process_array = (process **)malloc((*pipes + 1) * sizeof(process *));

        // If user only types enter, begin loop again
        if (user_input[0] == '\n')
        {
            continue;
        }

        if (*pipes != 0)
        {
            setup_process_wpipes(process_array, user_input, pipes);
        }
        else
        {
            countArgs(user_input, argnum);
            process_array[0] = malloc(sizeof(struct process));
            process_array[0]->args = malloc(sizeof(char *) * *argnum);
            process_array[0]->command = malloc(sizeof(char) * 64);
            process_array[0]->redirect_in = 0;
            process_array[0]->redirect_out = 0;
            process_array[0]->input_file = malloc(sizeof(char) * 64);
            process_array[0]->output_file = malloc(sizeof(char) * 64);

            setup_process(process_array[0], user_input);

            if (strcmp(process_array[0]->command, "exit") == 0)
            {

                if (*argnum != 1)
                {
                    printf("Warning: exit takes no args \n");
                }
                else
                {
                    return 0;
                }
            }
            else if (strcmp(process_array[0]->command, "cd") == 0)
            {
                if (*argnum != 2)
                {
                    printf("Warning: cd takes only 1 argument \n");
                }
                else
                {

                    int result = chdir(process_array[0]->args[1]);

                    if (result != 0)
                    {
                        perror("chdir");
                    }
                }
            }
            else
            {
                exec_cmd(process_array[0]);
            }
        }
    }

    return 0;
}
