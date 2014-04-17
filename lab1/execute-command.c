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
    if(c->type == SIMPLE_COMMAND)
    {
        c->status = fdwexecvp(c->u.word, c->input, c->output);
    }
    else if(c->type == AND_COMMAND)
    {
        int s = execute_node(c->u.command[0]);
        if(!s)
            c->status = execute_node(c->u.command[1]);
        else
            c->status = s;
    }
    else if(c->type == OR_COMMAND)
    {
        int s = (execute_node(c->u.command[0]));
        if(s)
            c->status = (execute_node(c->u.command[1]));
        else
            c->status = s;
    }
    else if(c->type == SEQUENCE_COMMAND)
    {
        execute_node(c->u.command[0]);
        c->status = execute_node(c->u.command[1]);
    }
    else if(c->type == PIPE_COMMAND)
    {
        int p1, p2, s, fd[2];
        if (pipe(fd) < 0) error(1,0,"Unable to create pipe");

        p1 = fork();
        if (p1 == 0) {
            close(fd[0]);
            if (dup2(fd[1], 1) < 0) error(1,0,"Unable to dup2");
            s = execute_node(c->u.command[0]);
            close(fd[1]);
            _exit(s);
        }
        else if (p1 < 0) error(1,0,"Unable to fork a process for pipe");
        p2 = fork();
        if (p2 == 0) {
            close(fd[1]);
            if (dup2(fd[0], 0) < 0) error(1,0,"Unable to dup2");
            s = execute_node(c->u.command[1]);
            close(fd[0]);
            _exit(s);
        }
        else if (p2 < 0) error(1,0,"Unable to fork a process for pipe");
        close(fd[1]);
        close(fd[0]);
        waitpid(p1, &s, 0);
        waitpid(p2, &s, 0);
        c->status = WEXITSTATUS(s);
    }
    else if(c->type == SUBSHELL_COMMAND)
    {
        c->status = execute_node(c->u.subshell_command);
    }
    
    return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
    if (!time_travel) {
        execute_node(c);
    }
}
