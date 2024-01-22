This project is to practice building a UNIX style shell from scratch, following the specification from the [KDLP course](https://kdlp.underground.software/course/fall2023/assignments/P0.md).

```
    lvl 0:
        The shell prints a prompt, informing the user about which absolute path they are currently in (see man 3 getcwd) followed by a $ and a space (e.g. /your/current/directory$).
        The shell then prints a new line and exits without any user interaction.
    lvl 1:
        The shell reads lines of user input, but doesn't need to do anything with them. It just prints a new prompt before each line.
        The shell loops until it gets EOF from user input (ctrl+d).
        The shell exits with code 0.
    lvl 2:
        If the user types any text, the shell prints "Unrecognized command" (but does not exit the loop).
        However, if the user just hits enter without typing anything, no error message is printed.
    lvl 3:
        The shell splits the line of input into pieces delimited by whitespace characters (see man 3 isspace).
        Instead of just printing "Unrecognized command" the shell shall include the name of the program in the error message (e.g. if the user types cat shell.c, the shell prints "Unrecognized command: cat").
    lvl 4:
        The shell supports a few builtin commands (exit, cd and exec). If the first piece of the input is not "exit" or "cd" or "exec" it will still print the unrecognized command message, otherwise:
            exit: takes no arguments (prints error if they are provided) and closes the shell (return value of 0)
            cd: takes exactly one argument (otherwise it prints usage info) and changes the working directory of the shell process (see man 2 chdir). If chdir does not accept the path provided, an error message including a description of the errno is printed. The new working directory is reflected in the prompt.
            exec: takes at least one argument (maybe more) and replaces the shell with an instance of the command specified (see man 3 execv). If execution fails, an error message is printed including a description of the errno, and the shell continues. Otherwise, having been replaced, the shell does not return.
    lvl 5:
        The shell supports running executable files as commands within child processes. If the first piece of the input looks like a path (starts with . or /) a child process is created (see man 2 fork) and the command specified by the first argument is executed within the child using the provided arguments (see man 3 execv) similar to the exec builtin. If executing the command fails, the child process prints an error message including a description of the errno (don't forget to exit the child process). The shell waits for the child to finish running before printing the next prompt (see man 2 waitpid).
    lvl 6:
        In the case that the user types something that isn't a path or a builtin, before printing the unrecognized command error, the shell checks whether a file with that name exists in each of the directories listed in the PATH environment variable in order (see man 3 getenv and man 2 stat). If a file, with that name is found, the search can stop and that file is executed with arguments in a child process. Only if no file is found in any of the directories, should the unrecognized command error be printed (NOTE: you must do the path searching manually, and cannot rely on a member of the exec family that does path searching automatically e.g. execvp).
    lvl 7:
        Before processing the entered commands, the shell performs home directory substitution on the pieces (command name or arguments) that start with a ~.
            The shell determines a username string by taking a substring of the piece after the ~ until the end of the string or the first /, whichever comes first.
            If the username string is empty, then the ~ is replaced with the value of the HOME environment variable (see man 3 getenv).
            If the username string is not empty, the shell attempts to locate the the user with that username and, if successful, replaces the ~ and the username substring with their home directory (see man 3 getpwnam).
            If getpwnam does not locate such a user, the shell leaves the piece unmodified.
    lvl 8:
        As the shell is processing the commands and arguments, if it finds a < or >, it skips any whitespace characters and attempts to treat the next piece of the input as a filename for redirection. If there is no filename before the end of the string, an error message is printed.
        A command can have more than one redirection (even more than one of the same type). If there are multiple of the same redirections, then the right-most one takes precedence.
        A redirection beginning with < causes the shell to open the corresponding file and replace the stdin of the child process with the file descriptor of the open file (see man 2 dup).
        A redirection beginning with > behaves similarly, but it replaces the stdout of the process.
    lvl 9:
        The shell supports the | (pipe) operator to chain multiple commands and their inputs and outputs.
        Each command separated by a | is spawned as its own child process. The shell can handle more than one pipe.
        The shell creates a unidirectional pipe (see man 2 pipe) for each | and redirects the stdout from the left command to the writing end of the pipe and redirects the stdin of the right command to the reading end of the pipe.
        Any file redirections specified by the user take precedence over any implied redirections of the |.

```