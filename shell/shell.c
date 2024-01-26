#include <unistd.h>
#include <stdio.h>
#include <string.h>

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

int main(int argc, char const *argv[])
{

    while (1)
    {
        char user_input[1024];
        char og_input[1024];
        char cwd[1024];

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        fgets(user_input, sizeof(user_input), stdin);

        strcpy(og_input, user_input);

        if (user_input)
        {

            // get rid of trailing space
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
            else if (strcmp(split_user_input, "exec") == 0)
            {
                if (argnum < 2)
                {
                    printf("exec takes 1 or more arguments: path, arg1, arg2, ... \n");
                }
                else
                {
                    char *args[1024];

                    int i = 0;

                    while (1)
                    {
                        split_user_input = strtok(NULL, " ");

                        if (split_user_input == NULL)
                        {
                            break;
                        }
                        else
                        {
                            split_user_input[strcspn(split_user_input, "\n")] = 0;
                            args[i] = split_user_input;

                            i++;
                        }
                    }

                
                    int result = execv(args[0], args);

                    if (result == -1)
                    {
                        perror("execv");
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            else
            {

                printf("Unrecognized command: %s \n", split_user_input);
            }
        }
    }

    return 0;
}
