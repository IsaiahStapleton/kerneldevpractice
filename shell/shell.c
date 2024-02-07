#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

char user_input[1024];
char cwd[1024];
int argnum;

typedef struct process
{
    char *command;
    char **args;

} process;

int countArgs(char *input)
{
    int i;

    int argnum = 1;

    for (i = 0; i < strlen(input); i++)
    {
        if (input[i] == ' ')
        {
            argnum++;
        }
    }

    return argnum;
}

void setup_process(process *process)
{
    int i = 0;

    if (user_input[0] == '.')
    {
        user_input[0] = ' ';
    }

    if (user_input[1] == '/' || user_input[0] == '/')
    {
        process->command = "exec";

        char *p = strtok(user_input, " ");

        while (p != NULL)
        {
            process->args[i++] = p;
            p = strtok(NULL, " ");

            if (p == NULL)
            {
                char *linep = strchr(process->args[i - 1], '\n');

                if (linep != NULL)
                {
                    *linep = '\0';
                }
            }
        }
    }
    else
    {
        char *p = strtok(user_input, " ");

        process->command = p;


        while (p != NULL)
        {
            process->args[i++] = p;
            p = strtok(NULL, " ");

            if (p == NULL)
            {
                char *linep = strchr(process->args[i - 1], '\n');

                if (linep != NULL)
                {
                    *linep = '\0';
                }
            }
        }
    }
}

int main(int argc, char const *argv[])
{

    while (1)
    {

        struct process parent;

        parent.args = malloc(sizeof(char *) * argnum);
        parent.command = malloc(sizeof(char) * 64);

        int status;

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        fgets(user_input, sizeof(user_input), stdin);

        // todo: Dynamically allocate memory for argnum
        argnum = countArgs(user_input);

        setup_process(&parent);

        // Remove newline character on last element in parent.args array

        // If user only types enter, begin loop again
        if (user_input[0] == '\n')
        {
            continue;
        }

        if (strcmp(parent.command, "exit") == 0)
        {

            if (argnum != 1)
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
            if (argnum != 2)
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
        else if (strcmp(parent.command, "exec") == 0 || user_input[0] == '/')
        {

            // printf("about to fork \n");
            pid_t pid = fork();

            if (pid == -1)
            {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                int result = execv(parent.args[0], parent.args);

                if (result == -1)
                {
                    perror("execv");
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

                // Parent process
                // printf("Parent Process: PID = %d, Child PID = %d\n", getpid(), pid);
                continue;
            }
        }
        else
        {

            pid_t pid = fork();

            if (pid == -1)
            {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                int result = execvp(parent.command, parent.args);

                if (result == -1)
                {
                    perror("execvp");
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
                else
                {
                    continue;
                }
            }

            printf("Unrecognized command: %s \n", parent.command);
        }
    }

    return 0;
}
