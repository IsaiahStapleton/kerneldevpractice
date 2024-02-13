#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char user_input[1024];
char cwd[1024];
char buffer[128];

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

void setup_process(process *process, char *user_input)
{
    int i = 0;

    char *p = strtok(user_input, " ");

    if (user_input[0] == '.')
    {
        user_input[0] = ' ';
    }

    if (user_input[1] == '/' || user_input[0] == '/')
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

void exec_cmd_wpipes(process **process)
{

    int status;
    int result;
    char cmd_output[1024];

    int current_process = 0;

    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) // Child
    {

        // Close read
        close(pipefd[0]);

        // Redirect stdout to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Handle redirection
        if (process[current_process]->redirect_in != 0)
        {
            redirect_in(process[current_process]);
        }
        else if (process[current_process]->redirect_out != 0)
        {
            redirect_out(process[current_process]);
        }

        // Execute command
        result = execvp(process[current_process]->args[0], process[current_process]->args);

        // Close write
        close(pipefd[1]);

        // Error checking for execvp
        if (result == -1)
        {
            perror("execvp");
            printf("Unrecognized command: %s \n", process[current_process]->command);

            exit(EXIT_FAILURE);
        }
    }
    else // Parent
    {
        // Close write
        close(pipefd[1]);

        // Read in output of execvp from last command
        read(pipefd[0], cmd_output, sizeof(cmd_output));

        // Close read
        close(pipefd[0]);

        handle_cmd_output(process, cmd_output, current_process);

        result = execvp(process[current_process + 1]->args[0], process[current_process + 1]->args);

        if (result == -1)
        {
            perror("execvp");
            printf("Unrecognized command: %s \n", process[current_process + 1]->command);

            exit(EXIT_FAILURE);
        }

        pid_t wait_pid = waitpid(pid, &status, 0);

        if (wait_pid == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
}

void setup_process_wpipes(process **process, char *user_input, int *pipes)
{
    int num_of_processes = *pipes + 1;
    // Keep track of which process is executing
    // int current_process = 0;
    int *argnum = malloc(sizeof(int));

    char *input = strtok(user_input, "|");

    // Setup each individual process
    for (size_t i = 0; i < num_of_processes; i++)
    {
        process[i] = malloc(sizeof(struct process));
        process[i]->command = malloc(sizeof(char) * 64);
        process[i]->redirect_in = 0;
        process[i]->redirect_out = 0;
        process[i]->input_file = malloc(sizeof(char) * 64);
        process[i]->output_file = malloc(sizeof(char) * 64);

        input = remove_space(input);

        countArgs(input, argnum);

        process[i]->args = malloc(sizeof(char *) * *argnum);

        setup_process(process[i], input);

        if (strcmp(process[i]->command, "exit") == 0)
        {

            if (*argnum != 1)
            {
                printf("Warning: exit takes no args \n");
            }
            // else
            // {
            //     return 0;
            // }
        }
        else if (strcmp(process[i]->command, "cd") == 0)
        {
            if (*argnum != 2)
            {
                printf("Warning: cd takes only 1 argument \n");
            }
            else
            {

                int result = chdir(process[i]->args[0]);

                if (result != 0)
                {
                    perror("chdir");
                }
            }
        }
        else
        {
            exec_cmd_wpipes(process);
        }
    }
}

void handle_cmd_output(process **process, char cmd_output[], int current_process)
{

    int *argnum = malloc(sizeof(int));

    char *input = strtok(user_input, "|");

    process[current_process + 1] = malloc(sizeof(struct process));
    process[current_process + 1]->command = malloc(sizeof(char) * 64);
    process[current_process + 1]->redirect_in = 0;
    process[current_process + 1]->redirect_out = 0;
    process[current_process + 1]->input_file = malloc(sizeof(char) * 64);
    process[current_process + 1]->output_file = malloc(sizeof(char) * 64);

    input = remove_space(input);

    countArgs(input, argnum);

    process[current_process + 1]->args = malloc(sizeof(char *) * *argnum);

    setup_process(process[current_process + 1], input);
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

                    int result = chdir(process_array[0]->args[0]);

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
