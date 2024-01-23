#include <unistd.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{

    char cwd[1024];
    char user_input[1024];

    while (1)
    {

        getcwd(cwd, 1024);

        printf("%s$ ", cwd);

        if (fgets(user_input, sizeof(user_input), stdin))
        {
            if (user_input[0] == '\n')
            {
                continue;
            }
            
            printf("Unrecognized command \n");
        } 
        

    }

    return 0;
}
