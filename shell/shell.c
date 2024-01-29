#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

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

void shell()
{
}

int main(int argc, char const *argv[])
{

    while (1)
    {
        char user_input[1024];
        char og_input[1024];
        char cwd[1024];

        int status;

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        fgets(user_input, sizeof(user_input), stdin);

        strcpy(og_input, user_input);

        char *split_user_input = strtok(user_input, " ");

        // Remove the newline character
        split_user_input[strcspn(split_user_input, "\n")] = 0;

        if (og_input[0] == '.' || og_input[0] == '/' || strcmp(split_user_input, "exec") == 0)
        {

            if (user_input[0] == '.')
            {

                // Shift the characters to the left by one position
                for (int i = 0; i < strlen(user_input); i++)
                {
                    user_input[i] = user_input[i + 1];
                }

            
            }
            

            printf("about to fork \n");
            pid_t pid = fork();

            if (pid == -1)
            {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                printf("Child Process: PID = %d\n", getpid());

                if (user_input)
                {

                    // get rid of trailing space
                    // todo: change to remove all spaces
                    if (og_input[strlen(og_input) - 2] == ' ')
                    {
                        og_input[strlen(og_input) - 2] = '\0';
                    }

                    int argnum = countArgs(og_input);

                    // printf("ARGNUM: %i \n", argnum);

                    if (argnum < 2)
                    {
                        printf("exec takes 1 or more arguments: path, arg1, arg2, ... \n");
                    }
                    else
                    {
                        char *args[1024];

                        int i = 0;

                        if (strcmp(split_user_input, "exec") == 0)
                        {
                            split_user_input = strtok(NULL, " ");
                        }

                        while (1)
                        {
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

                        printf("args[0]: %s \n", args[0]);

                        int result = execv(args[0], args);

                        if (result == -1)
                        {
                            perror("execv");
                            exit(EXIT_FAILURE);
                            printf("This shouldn't print");
                        }
                    }
                }
            }
            else
            {
                pid_t wait_pid = waitpid(pid, &status, 0);

                // Parent process
                printf("Parent Process: PID = %d, Child PID = %d\n", getpid(), pid);
                continue;
            }
        }

        if (user_input)
        {

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
            // else if (strcmp(split_user_input, "exec") == 0)
            // {
            //     if (argnum < 2)
            //     {
            //         printf("exec takes 1 or more arguments: path, arg1, arg2, ... \n");
            //     }
            //     else
            //     {
            //         pid_t pid = fork();

            //         char *args[1024];

            //         int i = 0;

            //         if (pid == -1)
            //         {
            //             perror("fork failed");
            //             return 1;
            //         }
            //         else if (pid == 0) // Child process
            //         {
            //             while (1)
            //             {
            //                 split_user_input = strtok(NULL, " ");

            //                 if (split_user_input == NULL)
            //                 {
            //                     break;
            //                 }
            //                 else
            //                 {
            //                     split_user_input[strcspn(split_user_input, "\n")] = 0;
            //                     args[i] = split_user_input;

            //                     i++;
            //                 }
            //             }

            //             int result = execv(args[0], args);

            //             if (result == -1)
            //             {
            //                 perror("execv");
            //                 exit(EXIT_FAILURE);
            //             }
            //         }
            //         else
            //         {
            //         }
            //     }
            // }

            else
            {

                printf("Unrecognized command: %s \n", split_user_input);
            }
        }
    }

    return 0;
}
