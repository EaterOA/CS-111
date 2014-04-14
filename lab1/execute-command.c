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

int fexecvp(char** word)
{
    int p = fork();
    int status;
    if (p == 0)
        execvp(word[0], word);
    if (p > 0)
        if (waitpid(p, &status, 0) < 0)
            error(1,0, "Unable to wait for pid %d", p);
    if (p < 0)
        error(1,0, "Unable to fork");
    return status;
}

bool execute_node(command_t c)
{
    int in, out;
    int in_copy, out_copy;
    bool success = 0;

    if (c->type == SIMPLE_COMMAND) {
        if (c->input) {
            if ((in_copy = dup(0)) < 0)
                error(1,0, "Cannot dup input!");
            if ((in = open(c->input, O_RDONLY)) < 0)
                error(1,0, "Cannot open %s!", c->input);
            if (dup2(in, 0) < 0)
                error(1,0, "Cannot dup2 %s to stdin!", c->input);
        }
        if (c->output) {
            if ((out_copy = dup(1)) < 0)
                error(1,0, "Cannot dup output!");
            if ((out = open(c->output, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0)
                error(1, 0, "Cannot open %s!", c->output);
            if (dup2(out, 1) < 0)
                error(1,0, "Cannot dup2 %s to stdout!", c->output);
        }
        success = (fexecvp(c->u.word) == 0);
        if (c->input) {
            close(in);
            dup2(in_copy, 0);
            close(in_copy); 
        }
        if (c->output) {
            close(out);
            dup2(out_copy, 1);
            close(out_copy);
        }
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
