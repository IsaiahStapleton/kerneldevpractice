#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char user_input[1024];
char cwd[1024];

typedef struct process
{
    char *command;
    char *filename;
    char **args;
    int redirect_in;
    int redirect_out;
    int pipes;

} process;

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
            *num = *num+1;
        }
    }

}

int redirect_in(process *process, int i)
{

    int file = open(process->filename, O_RDONLY);

    int original_stdin = dup(STDIN_FILENO);

    if (file == -1)
    {
        perror("Error opening file");
    }

    if (dup2(file, STDIN_FILENO) == -1)
    {
        perror("Error redirecting stdin");
    }

    close(file);


    char *current_word = malloc(sizeof(char) * 64);

    int j = 0;

    int get_char;
    while ((get_char = getchar()) != EOF)
    {

        // if ((get_char = getchar()) == EOF)
        // {
        //     process->args[i] = *current_word;
        //     i++;
        //     break;
        // }

        if (get_char == 10)
        {
            current_word[j] = '\0';
            process->args[i] = malloc(sizeof(char) * 64);
            strcpy(process->args[i], current_word);
            i++;
            current_word[0] = '\0';
            j = 0;
        }
        else if (get_char != 32)
        {
            current_word[j] = get_char;
            j++;
        }
    }

    // Restore stdin to its original file descriptor
    if (dup2(original_stdin, STDIN_FILENO) == -1)
    {
        perror("Error restoring stdin");
    }
    
    close(original_stdin);


    return i;
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
        strcpy(process->command, p);
    }

    while (p != NULL)
    {

        if (strcmp(p, "<") == 0)
        {
            process->redirect_in++;
            p = strtok(NULL, " ");
            process->filename = remove_newline(p);
            i = redirect_in(process, i);
        }
        else if (strcmp(p, ">") == 0)
        {
            process->redirect_out++;
            p = strtok(NULL, " ");
            process->filename = p;
            i++;
        }
        else if (strcmp(p, "|") == 0)
        {
            process->pipes++;
        }
        else
        {
            process->args[i] = malloc(sizeof(char) * 64);
            process->args[i++] = remove_newline(p);
        }

        p = strtok(NULL, " ");

    }
}

int main(int argc, char const *argv[])
{

    while (1)
    {
        int status;
        struct process parent;

        int *argnum = malloc(sizeof(int));

        
        parent.command = malloc(sizeof(char) * 64);
        parent.redirect_in = 0;
        parent.redirect_out = 0;
        parent.pipes = 0;
        parent.filename = malloc(sizeof(char) * 64);



        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        fgets(user_input, 64, stdin);


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
        else if (strcmp(parent.command, "exec") == 0 || user_input[0] == '/')
        {

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
