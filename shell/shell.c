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
    int pipes;
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
        else if (strcmp(p, "|") == 0)
        {
            process->pipes++;
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
        struct process parent;

        int *argnum = malloc(sizeof(int));

        parent.command = malloc(sizeof(char) * 64);
        parent.redirect_in = 0;
        parent.redirect_out = 0;
        parent.pipes = 0;
        parent.input_file = malloc(sizeof(char) * 64);
        parent.output_file = malloc(sizeof(char) * 64);

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);
        fflush(stdout);

        fgets(user_input, sizeof(user_input), stdin);

        countArgs(user_input, argnum);

        parent.args = malloc(sizeof(char *) * *argnum);

        // If user only types enter, begin loop again
        if (user_input[0] == '\n')
        {
            continue;
        }

        setup_process(&parent, user_input);

        if (strcmp(parent.command, "exit") == 0)
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
        else if (strcmp(parent.command, "cd") == 0)
        {
            if (*argnum != 2)
            {
                printf("Warning: cd takes only 1 argument \n");
            }
            else
            {

                int result = chdir(parent.args[0]);

                if (result != 0)
                {
                    perror("chdir");
                }
            }
        }
        else
        {

            exec_cmd(&parent);
        }
    }

    return 0;
}
