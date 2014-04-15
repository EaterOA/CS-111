// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int
command_status (command_t c)
{
    return c->status;
}

bool fdwexecvp(char** word, char* input, char* output)
{
    int fd_in, fd_out;
    int status;
    int p = fork();
    if (p == 0) {
        if (input) {
            if ((fd_in = open(input, O_RDONLY)) < 0)
                error(1,0,"Unable to open %s as input", input);
            if (dup2(fd_in, 0) < 0)
                error(1,0,"Unable to dup2 %s as stdin", input);
        }
        if (output) {
            if ((fd_out = open(output, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0)
                error(1, 0, "Unable to open %s as output", output);
            if (dup2(fd_out, 1) < 0)
                error(1,0,"Unable to dup2 %s to stdout", output);
        }
        execvp(word[0], word);
        error(1,0,"Unable to execvp %s", word[0]);
    }
    if (p > 0)
        if (waitpid(p, &status, 0) < 0)
            error(1,0, "Unable to wait for pid %d", p);
    if (p < 0)
        error(1,0, "Unable to fork");
    return WEXITSTATUS(status) == 0;
}

bool execute_node(command_t c)
{
    bool success = 0;

    if (c->type == SIMPLE_COMMAND) {
        success = fdwexecvp(c->u.word, c->input, c->output);
    }
    return success;
}

void
execute_command (command_t c, bool time_travel)
{
    if (!time_travel) {
        execute_node(c);
    }
}
