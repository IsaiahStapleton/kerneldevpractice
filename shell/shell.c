#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

char user_input[1024];
char og_input[1024];
char cwd[1024];

char parsed_input[1024][1024];

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

void parseArgs()
{
    // j is current loc in string array, is loc in user input string
    int j = 0;
    int i = 0;
    int k = 0;

    while (j != -1)
    {

        if (user_input[0] == '.')
        {
            user_input[0] == ' ';
        }
        
        // exec /bin/ls -l

        if (user_input[i] == ' ')
        {
            j++;
            i++;
            k = 0;
            
            continue;
        }
        else if (user_input[i] == '\0')
        {
            j = -1;
            break;
        }
        else
        {
            parsed_input[j][k] = user_input[i];
            k++;
            i++;
        }

    }
}

int main(int argc, char const *argv[])
{

    while (1)
    {

        int status;

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        fgets(user_input, sizeof(user_input), stdin);

        strcpy(og_input, user_input);

        parseArgs();

        // char *split_user_input = strtok(user_input, " ");

        // // Remove the newline character
        // split_user_input[strcspn(split_user_input, "\n")] = 0;

        // get rid of trailing space
        // todo: change to remove all spaces
        if (og_input[strlen(og_input) - 2] == ' ')
        {
            og_input[strlen(og_input) - 2] = '\0';
        }

        int argnum = countArgs(og_input);

        char *split_user_input = strtok(user_input, " ");

        if (user_input[0] == '\n')
        {
            continue;
        }

        // Remove the newline character
        split_user_input[strcspn(split_user_input, "\n")] = 0;

        if (strcmp(split_user_input, "exit") == 0)
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
        else if (strcmp(split_user_input, "cd") == 0)
        {
            if (argnum != 2)
            {
                printf("Warning: exit takes only 1 argument \n");
            }
            else
            {
                // Advance string to second argument && remove newline char
                split_user_input = strtok(NULL, " ");
                split_user_input[strcspn(split_user_input, "\n")] = 0;

                int result = chdir(split_user_input);

                if (result != 0)
                {
                    perror("chdir");
                }
            }
        }
        else if (strcmp(split_user_input, "exec") == 0 || og_input[0] == '.' || og_input[0] == '/')
        {

            if (user_input[0] == '.')
            {

                // Shift the characters to the left by one position to get rid of leading . char
                for (int i = 0; i < strlen(user_input); i++)
                {
                    user_input[i] = user_input[i + 1];
                }
            }

            // printf("about to fork \n");
            pid_t pid = fork();

            if (pid == -1)
            {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // printf("Child Process: PID = %d\n", getpid());

                // get rid of trailing space
                // todo: change to remove all spaces
                if (og_input[strlen(og_input) - 2] == ' ')
                {
                    og_input[strlen(og_input) - 2] = '\0';
                }

                char *args[1024];

                int i = 0;

                if (strcmp(split_user_input, "exec") == 0)
                {
                    split_user_input = strtok(NULL, " ");
                }

                while (1)
                {
                    printf("%s ", split_user_input);

                    if (split_user_input == NULL)
                    {
                        break;
                    }
                    else
                    {
                        split_user_input[strcspn(split_user_input, "\n")] = 0;

                        args[i] = split_user_input;

                        split_user_input = strtok(NULL, " ");

                        i++;
                    }
                }

                int result = execv(args[0], args);

                if (result == -1)
                {
                    perror("execv");
                    exit(EXIT_FAILURE);
                    printf("This shouldn't print");
                }
            }
            else
            {
                pid_t wait_pid = waitpid(pid, &status, 0);

                // Parent process
                // printf("Parent Process: PID = %d, Child PID = %d\n", getpid(), pid);
                continue;
            }
        }
        else
        {

            printf("Unrecognized command: %s \n", split_user_input);
        }
    }

    return 0;
}
