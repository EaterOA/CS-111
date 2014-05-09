// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include "darray.h"
#include <stdio.h>
#include <time.h>

//============================ Sequential execution ============================

static bool measure;
static int pfd[2];

void
parse_rusage(long int* u_sec, long int* u_msec, long int* s_sec, long int* s_msec, long int* rss, struct rusage rusage)
{
    *u_sec = rusage.ru_utime.tv_sec;
    *s_sec = rusage.ru_stime.tv_sec;
    long int u_usec = rusage.ru_utime.tv_usec;
    long int s_usec = rusage.ru_stime.tv_usec;
    *u_msec = (u_usec + 500)/1000;
    *s_msec = (s_usec + 500)/1000;
    if (*u_msec >= 1000) {
        *u_msec -= 1000;
        (*u_sec)++;
    }
    if (*s_msec >= 1000) {
        *s_msec -= 1000;
        (*s_sec)++;
    }
    *rss = rusage.ru_maxrss;
}

void redirect(int* fd_in, int* fd_out, char* input, char* output)
{
    if (input) {
        if ((*fd_in = open(input, O_RDONLY)) < 0)
            error(1,0,"Unable to open %s as input", input);
        if (dup2(*fd_in, 0) < 0)
            error(1,0,"Unable to dup2 %s as stdin", input);
    }
    if (output) {
        if ((*fd_out = open(output, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0)
            error(1, 0, "Unable to open %s as output", output);
        if (dup2(*fd_out, 1) < 0)
            error(1,0,"Unable to dup2 %s to stdout", output);
    }
}

int
fdwexec(command_t c)
{
    struct rusage rusage;
    long int rss, sec, msec, u_sec, u_msec, s_sec, s_msec;
    int fd_in, fd_out, s, p;

    p = fork();
    if (p == 0) {
        redirect(&fd_in, &fd_out, c->input, c->output);
        if (strcmp(c->u.word[0], "exec"))
            execvp(c->u.word[0], c->u.word);
        else
            execvp(c->u.word[1], c->u.word+1);
        error(1,0,"Unable to execvp %s", c->u.word[0]);
    }
    if (p > 0) {
        if (!measure) {
            if (waitpid(p, &s, 0) < 0)
                error(1,0, "Unable to wait for pid %d", p);
        }
        else {
            if (wait4(p, &s, 0, &rusage) < 0)
                error(1,0, "Unable to wait for pid %d", p);
            parse_rusage(&u_sec, &u_msec, &s_sec, &s_msec, &rss, rusage);
            sec = u_sec + s_sec;
            msec = u_msec + s_msec;
            if (msec >= 1000) {
                msec -= 1000;
                sec++;
            }
            char buf[50];
            sprintf(buf, "m %p %ld\n", c, rss);
            write(pfd[1], buf, strlen(buf));
            sprintf(buf, "t %p %ld%03ld\n", c, sec, msec);
            write(pfd[1], buf, strlen(buf));
        }
    }
    if (p < 0)
        error(1,0, "Unable to fork");
    return WEXITSTATUS(s);
}

int execute_node(command_t c)
{
    if(c->type == SIMPLE_COMMAND)
    {
        c->status = fdwexec(c);
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
        int s = execute_node(c->u.command[0]);
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
        int fd_in, fd_out, s, p;
        p = fork();
        if (p == 0) {
            redirect(&fd_in, &fd_out, c->input, c->output);
            s = execute_node(c->u.subshell_command);
            if (c->input) close(fd_in);
            if (c->output) close(fd_out);
            _exit(s);
        }
        if (p > 0)
            if (waitpid(p, &s, 0) < 0)
                error(1,0, "Unable to wait for pid %d", p);
        if (p < 0)
            error(1,0, "Unable to fork");
        c->status = WEXITSTATUS(s);
    }
    char buf[100];
    sprintf(buf, "s %p %d\n", c, c->status);
    write(pfd[1], buf, strlen(buf));

    return c->status;
}

void compute_time_rss(command_t c)
{
    if(c->type == SUBSHELL_COMMAND)
    {
        compute_time_rss(c->u.subshell_command);
        c->rss = c->u.subshell_command->rss;
        c->time = c->u.subshell_command->time;
    }
    else if(c->type == AND_COMMAND)
    {       
        compute_time_rss(c->u.command[0]);
        c->rss = c->u.command[0]->rss;
        c->time = c->u.command[0]->time;
     
        if(!c->u.command[0]->status)
        {
            compute_time_rss(c->u.command[1]);
            if(c->u.command[1]->rss > c->rss)
                c->rss = c->u.command[1]->rss;
            c->time += c->u.command[1]->time;
        }
    }
    else if(c->type == OR_COMMAND)
    {
        compute_time_rss(c->u.command[0]);
        c->rss = c->u.command[0]->rss;
        c->time = c->u.command[0]->time;
     
        if(c->u.command[0]->status)
        {
            compute_time_rss(c->u.command[1]);
            if(c->u.command[1]->rss > c->rss)
                c->rss = c->u.command[1]->rss;
            c->time += c->u.command[1]->time;
        }
    }
    else if(c->type == SEQUENCE_COMMAND || c->type == PIPE_COMMAND)
    {
        compute_time_rss(c->u.command[0]);
        c->rss = c->u.command[0]->rss;
        c->time = c->u.command[0]->time;
      
        compute_time_rss(c->u.command[1]);
        if(c->u.command[1]->rss > c->rss)
            c->rss = c->u.command[1]->rss;
        c->time += c->u.command[1]->time;
    }
    else //c->type == SIMPLE_COMMAND
    {
        return;
    }
}

int count_forks(command_t c)
{
    if(c->type == SUBSHELL_COMMAND)
    {
        return 1 + count_forks(c->u.subshell_command);
    }
    else if(c->type == PIPE_COMMAND)
    {
        int c1 = count_forks(c->u.command[0]);
        return 2 + c1 + count_forks(c->u.command[1]);
    }
    else if(c->type == AND_COMMAND)
    {
        int f = count_forks(c->u.command[0]);
        if(!c->u.command[0]->status)
        {
            f += count_forks(c->u.command[1]);
        }
        return f;
    }
    else if(c->type == OR_COMMAND)
    {
        int f = count_forks(c->u.command[0]);
        if(c->u.command[0]->status)
        {
            f += count_forks(c->u.command[1]);
        }
        return f;
    }
    else if(c->type == SEQUENCE_COMMAND)
    {
        return count_forks(c->u.command[0]) + count_forks(c->u.command[1]);
    }
    else //c->type == SIMPLE_COMMAND
    {
        return 1;
    }
}

void
print_resource_use(int forks, long int c_time, long int c_rss)
{
    struct rusage rusage;
    long int u_sec, u_msec, s_sec, s_msec, s_rss;
    long int maxvm = 0;
    char buf[100];
    FILE* f;
    
    getrusage(RUSAGE_SELF, &rusage);
    sprintf(buf, "/proc/%d/status", getpid());
    f = fopen(buf, "r");
    while (fgets(buf, 100, f)) {
        if (strncmp("VmPeak:", buf, 7) == 0) {
            sscanf(buf, "%*s %ld", &maxvm);
            break;
        }
    }
    parse_rusage(&u_sec, &u_msec, &s_sec, &s_msec, &s_rss, rusage);
    printf("Statistics:\n");
    printf("Shell user time: %ld.%03ld s\n", u_sec, u_msec);
    printf("Shell system time: %ld.%03ld s\n", s_sec, s_msec);
    printf("Shell peak RSS: %ld kb\n", s_rss);
    printf("Shell peak VM: %ld kb\n", maxvm);
    printf("Forks: %d\n", forks);
    printf("Script cpu time: %ld.%03ld s\n", c_time/1000, c_time%1000);
    printf("Script peak RSS: %ld kb\n", c_rss);
}

int
execute_sequential (command_stream_t s, bool m)
{
    int ret, i, mem_indent, time_indent;
    long int value;
    char tag, buf[100];
    FILE* p;
    command_t c, cur;

    measure = m;
    int forks = 0;
    long int c_time = 0;
    long int c_rss = 0;
    for (i = 1; (c = read_command_stream (s)); i++) {
        if (m) pipe(pfd);
        ret = execute_node(c);
        if (m) {
            close(pfd[1]);
            p = fdopen(pfd[0], "r");
            while (fscanf(p, " %c %ld %ld", &tag, &cur, &value) > 0) {
                if(tag == 'm')
                    cur->rss = value;
                else if(tag == 't')
                    cur->time = value;
                else if(tag == 's')
                    cur->status = (int)value;
            }
            fclose(p);
            forks += count_forks(c);
            compute_time_rss(c);
            c_time += c->time;
            c_rss = (c->rss > c_rss ? c->rss : c_rss);
            printf("# %d\n", i);
            sprintf(buf, "%ld", c->rss);
            mem_indent = strlen(buf);
            sprintf(buf, "%ld.%03ld", c->time/1000, c->time%1000);
            time_indent = strlen(buf);
            print_command(c, true, mem_indent, time_indent);
            printf("\n");
        }
    }
    
    if (m) {
        print_resource_use(forks, c_time, c_rss);
    }
    return ret;
}

//============================ Time travel execution ============================

typedef struct graph_node* graph_node_t;
struct graph_node
{
    command_t cmd;
    darray_t readlist;  //darray of char*
    darray_t writelist; //darray of char*
    darray_t after;     //darray of graph_node_t
    int pid;
    int dep;
};

graph_node_t init_graph_node(void)
{
    graph_node_t node = (graph_node_t)checked_malloc(sizeof(struct graph_node));
    node->cmd = NULL;
    node->readlist = darray_init();
    node->writelist = darray_init();
    node->after = darray_init();
    node->pid = 0;
    node->dep = 0;
    return node;
}

void free_graph_node(graph_node_t node)
{
    darray_free(node->readlist);
    darray_free(node->writelist);
    darray_free(node->after);
}

void construct_read_write_list(graph_node_t node, command_t cmd)
{
    if(!cmd)
        error(1, 0, "No command");
    else if(cmd->type == SIMPLE_COMMAND)
    {
        char** words = cmd->u.word + 1;
        while(*words != NULL)
        {
            if(**words != '-')
            {
                darray_push(node->readlist, *(words));
            }
            words++;
        }

        if(cmd->input)
            darray_push(node->readlist, cmd->input);
        if(cmd->output)
            darray_push(node->writelist, cmd->output);
    }
    else if(cmd->type == SUBSHELL_COMMAND)
    {
        command_t subshell_cmd = cmd->u.subshell_command; 
        construct_read_write_list(node, subshell_cmd);
        if(cmd->input)
            darray_push(node->readlist, cmd->input);
        if(cmd->output)
            darray_push(node->writelist, cmd->output);
    
    }
    else //all the other types
    {
        command_t c1 = cmd->u.command[0];
        construct_read_write_list(node, c1);
 
        command_t c2 = cmd->u.command[1];
        construct_read_write_list(node, c2);
    }
    
}

void construct_dependencies(darray_t g, graph_node_t node)
{
    size_t i;
    for(i = 0; i < g->count; i++)
    {
        graph_node_t cur = darray_get(g, i);
        int flag = 0;
        size_t j;
        for(j = 0; j < cur->readlist->count; j++)
        {
            size_t k;
            for(k = 0; k < node->writelist->count; k++)
            {
                if(strcmp(darray_get(cur->readlist, j),
                          darray_get(node->writelist, k)) == 0)
                     flag = 1;
            }
        }

        for(j = 0; j < cur->writelist->count; j++)
        {
            size_t k;
            for(k = 0; k < node->readlist->count; k++)
            {
                if(strcmp(darray_get(cur->writelist, j),
                          darray_get(node->readlist, k)) == 0)
                     flag = 1;
            }
            for(k = 0; k < node->writelist->count; k++)
            {
                if(strcmp(darray_get(cur->writelist, j),
                          darray_get(node->writelist, k)) == 0)
                     flag = 1;
            }
        }
        if(flag == 1)
        {
            node->dep++;
            darray_push(cur->after, node);
        }
    }
}

int
execute_time_travel (command_stream_t s)
{
    measure = false;

    //Construct graph
    darray_t g = darray_init(); //darray of graph_node_t
    command_t c;
    while ((c = read_command_stream (s))) {
        graph_node_t node = init_graph_node();
        node->cmd = c;
        construct_read_write_list(node, c);
        construct_dependencies(g, node);
        darray_push(g, node);
    }
    
    //Removing nonsources from graph
    size_t i, j;
    for (i = 0; i < g->count;) {
        graph_node_t node = (graph_node_t)darray_get(g, i);
        if (node->dep > 0)
            darray_remove_unordered(g, i);
        else
            i++;
    }
    
    //Execute sources
    for (i = 0; i < g->count; i++) {
        graph_node_t node = (graph_node_t)darray_get(g, i);
        int p = fork();
        if (p == 0) {
            int ret = execute_node(node->cmd);
            _exit(ret);
        }
        else if (p > 0)
            node->pid = p;
        else if (p < 0)
            error(1,0,"Unable to fork processes for sources");
    }
    
    //Poll sources until they're done, resolve dependencies, and add more sources
    int status;
    while (g->count > 0) {
        for (i = 0; i < g->count;) {
            graph_node_t node = (graph_node_t)darray_get(g, i);
            int p = waitpid(node->pid, &status, WNOHANG);
            if (p > 0) {
                darray_remove_unordered(g, i);
                for (j = 0; j < node->after->count; j++) {
                    graph_node_t next = (graph_node_t)darray_get(node->after, j);
                    next->dep--;
                    if (next->dep == 0) {
                        p = fork();
                        if (p == 0) {
                            int ret = execute_node(next->cmd);
                            _exit(ret);
                        }
                        else if (p > 0) {
                            next->pid = p;
                            darray_push(g, next);
                        }
                        else if (p < 0)
                            error(1,0,"Unable to fork processes for sources");
                    }
                }
                free_graph_node(node);
            }
            else if (p == 0)
                i++;
            else if (p < 0)
                error(1,0,"Unable to call waitpid on sources");
        }
    }
    
    return 0;
}
