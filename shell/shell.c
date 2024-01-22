#include <unistd.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char cwd[1024];

    getcwd(cwd, 1024);

    printf("%s$ ", cwd);

    printf("\n");

    return 0;
}
