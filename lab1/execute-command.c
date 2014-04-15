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

int fdwexecvp(char** word, char* input, char* output)
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
    return WEXITSTATUS(status);
}

int execute_node(command_t c)
{
    int status = 0;

    if (c->type == SIMPLE_COMMAND) {
        status = fdwexecvp(c->u.word, c->input, c->output);
    }
    else if(c->type == AND_COMMAND)
    {
        int c1 = (execute_node(c->u.command[0]));
        if(!c1)
            status = (execute_node(c->u.command[1]));
        else
            status = c1;
    }
    else if(c->type == OR_COMMAND)
    {
        int c1 = (execute_node(c->u.command[0]));
        if(c1)
            status = (execute_node(c->u.command[1]));
        else
            status = c1;
    }
    else if(c->type == SEQUENCE_COMMAND)
    {
        execute_node(c->u.command[0]);
        status = execute_node(c->u.command[1]);
    }
    else if(c->type == PIPE_COMMAND)
    {
        int in_cpy, out_cpy;
        int fd[2];
        in_cpy = dup(0);
        out_cpy = dup(1);
        pipe(fd);

        dup2(fd[1], 1);
        execute_node(c->u.command[0]);
        dup2(fd[0], 0);

        dup2(out_cpy, 1);
        status = execute_node(c->u.command[1]);

        dup2(in_cpy, 0);
        
        close(fd[0]);
        close(fd[1]);
        close(out_cpy);
        close(in_cpy);
    }
    
    return status;
}

void
execute_command (command_t c, bool time_travel)
{
    if (!time_travel) {
        execute_node(c);
    }
}
