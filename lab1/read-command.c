// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include <error.h>
#include "alloc.h"
#include "stack.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct command_node* command_node_t;

struct command_node
{
    command_t command;
    command_node_t next;
};

struct command_stream
{
    command_node_t head;
};

bool is_valid_token(char c)
{
   if(c >= 'A' && c <= 'Z')
        return true;

   if(c >= 'a' && c <= 'z')
        return true;

   if(c >= '0' && c <= '9')
        return true;

   if(c == '!' || c == '%' || c == '+' ||
      c == ',' || c == '-' || c == '.' ||
      c == '/' || c == ':' || c == '@' ||
      c == '^' || c == '_')
        return true;

   return false;
}

command_t allocate_command()
{
    command_t cmd = (command_t)checked_malloc(sizeof(struct command));
    cmd->input = NULL;
    cmd->output = NULL;
    return cmd;
}

command_node_t allocate_command_node()
{
    command_node_t cmdNode = (command_node_t)checked_malloc(sizeof(struct command_node));
    cmdNode->command = NULL;
    cmdNode->next = NULL;
    return cmdNode;
}

void* error_ret(int* err)
{
    *err = 1;
    return NULL;
}

void skipspace(char** c)
{
    while (**c == ' ') (*c)++;
}

void skipcomments(char** c)
{
    if (**c == '#')
        while (**c && **c != '\n')
            (*c)++;
}

void skipwhitespace(char** c)
{
    while (**c == ' ' || **c == '\n' || **c == '#') {
        skipcomments(c);
        (*c)++;
    }
}

char precedes(command_t a, command_t b)
{
    //Checks if b precedes a
    //Order: {PIPE}, {AND, OR}, {SEQUENCE}
    if (b->type == PIPE_COMMAND) {
        return a->type != PIPE_COMMAND;
    }
    else if (b->type == AND_COMMAND || b->type == OR_COMMAND) {
        return a->type == SEQUENCE_COMMAND;
    }
    return 0;
}

size_t parse_word(char** c, char* buf, size_t* buf_size, size_t* max_size)
{
    size_t read = 0;
    skipspace(c);
    while (is_valid_token(**c))
    {
        if (*buf_size == *max_size)
            buf = (char*)checked_grow_alloc(buf, max_size);

        buf[*buf_size] = (**c);
        read++;
        (*buf_size)++;
        (*c)++;
    }

    return read;
}

command_t parse_simple_command(char** c, int* err)
{
    skipspace(c);
    skipcomments(c);

    command_t com = allocate_command();
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;   

    int num = 0;
    size_t buf_size = 0, max_size = 128;
    size_t in_buf_size = 0, in_max_size = 32;
    size_t out_buf_size = 0, out_max_size = 32;
    char* buf = (char*)checked_malloc(max_size * sizeof(char));
    char* in_buf = (char*)checked_malloc(in_max_size * sizeof(char));
    char* out_buf = (char*)checked_malloc(out_max_size * sizeof(char));


    for (;;)
    {
        size_t len = parse_word(c, buf, &buf_size, &max_size);
        if (len > 0) {
            buf[buf_size] = '\0';
            buf_size++;
            num++;  
        }
        
        char ch = **c;
        if (ch == ' ') {
            skipspace(c);
        }
        else if (ch == '<')
        {
            if (buf_size == 0) error_ret(err);
            (*c)++;
            size_t read = parse_word(c, in_buf, &in_buf_size, &in_max_size);
            if (read == 0) error_ret(err);
            in_buf[in_buf_size] = '\0';
            in_buf_size++;
            com->input = in_buf;
            skipspace(c);
        }
        else if (ch == '>')
        {
            if (buf_size == 0) error_ret(err);
            (*c)++;
            size_t read = parse_word(c, out_buf, &out_buf_size, &out_max_size);
            if (read == 0) error_ret(err);
            out_buf[out_buf_size] = '\0';
            out_buf_size++;
            com->output = out_buf;
            skipspace(c);
        }
        else break;
    }

    if(buf_size == 0)
        return NULL;

    buf[buf_size] = '\0';
    com->u.word = (char**)checked_malloc((num+1) * sizeof(char*));
    char* curr = buf;
    com->u.word[0] = curr;
    int i;
    for(i = 1; i < num; i++)
    {
        while(*curr != '\0') 
            curr++;

        curr++;
        com->u.word[i] = curr;
    }
    com->u.word[num] = NULL;
    return com;
}
    
command_t parse_root_command(char** c, char isTopLevel, int* err)
{
    stack_t oprd = stack_init();
    stack_t optr = stack_init();
    
    //Initialize first found command
    skipwhitespace(c);
    if (!**c) return NULL;
    command_t cmd = parse_simple_command(c, err);
    if (*err) return error_ret(err);
    if (!cmd) {
        if (**c == '(') {
            (*c)++;
            cmd = allocate_command();
            cmd->type = SUBSHELL_COMMAND;
            cmd->u.subshell_command = parse_root_command(c, 0, err);
            if (!cmd->u.subshell_command) return error_ret(err);
        }
        else return error_ret(err);
    }
    stack_push(oprd, cmd);
    
    for (;;) {
        skipcomments(c);
        char ch = **c;
        
        //Determine operator (or break or error)
        command_t link = allocate_command();
        if (ch == '&') {
            (*c)++;
            if (*(*c)++ != '&') return error_ret(err);
            link->type = AND_COMMAND;
        }
        else if (ch == '|') {
            (*c)++;
            if (**c == '|') {
                (*c)++;
                link->type = OR_COMMAND;
            }
            else {
                link->type = PIPE_COMMAND;
            }
        }
        else if (ch == ';') {
            (*c)++;
            link->type = SEQUENCE_COMMAND;
        }
        else if (ch == '\n') {
            (*c)++;
            skipspace(c);
            skipcomments(c);
            if (!**c || **c == '\n') {
                if (!isTopLevel) return error_ret(err);
                break;
            }
            link->type = SEQUENCE_COMMAND;
        }
        else if (ch == ')') {
            if (isTopLevel) return error_ret(err);
            (*c)++;
            skipspace(c);
            break;
        }
        else if (!ch) {
            if (!isTopLevel) return error_ret(err);
            break;
        }
        else return error_ret(err);
        
        //Keep popping until link can be placed in stack
        while (stack_count(optr) != 0 && !precedes((command_t)stack_top, link)) {
            command_t t = (command_t)stack_pop(optr);
            if (stack_count(oprd) < 2) error(1, 0, "Shit happened");
            t->u.command[1] = (command_t)stack_pop(oprd);
            t->u.command[0] = (command_t)stack_pop(oprd);
            stack_push(oprd, t);
        }
        stack_push(optr, link);
        
        //Pull next operand
        skipwhitespace(c);
        command_t next = parse_simple_command(c, err);
        if (*err) return error_ret(err);
        if (!next) {
            if (**c == '(') {
                (*c)++;
                next = allocate_command();
                next->type = SUBSHELL_COMMAND;
                next->u.subshell_command = parse_root_command(c, 0, err);
                if (!next->u.subshell_command) return error_ret(err);
            }
            else return error_ret(err);
        }
        stack_push(oprd, next);
    }
    
    //Consolidate all operands and operators
    while (stack_count(optr) != 0 && c) {
        command_t t = (command_t)stack_pop(optr);
        if (stack_count(oprd) < 2) error(1, 0, "Crap happened");
        t->u.command[1] = (command_t)stack_pop(oprd);
        t->u.command[0] = (command_t)stack_pop(oprd);
        stack_push(oprd, t);
    }
    if (stack_count(optr) != 0 || stack_count(oprd) != 1) return error_ret(err);
    
    cmd = stack_pop(oprd);
    
    stack_free(oprd);
    stack_free(optr);
    
    return cmd;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    //Construct command_stream
    command_stream_t cs = (command_stream_t)checked_malloc(sizeof(struct command_stream));
    
    //Read the entire script into a buf
    size_t buf_size = 0, max_size = 512;
    char* buf = (char*)checked_malloc(max_size * sizeof(char));
    for (;;) {
        int c = get_next_byte(get_next_byte_argument);
        if (buf_size == max_size) 
            buf = (char*)checked_grow_alloc(buf, &max_size);
        if (c == -1) {
            buf[buf_size++] = '\0';
            break;
        }
        buf[buf_size++] = (char)c;
    }
    
    //Construct nodes from each root command
    char* ptr = buf, *marker;
    int err = 0;
    cs->head = allocate_command_node();
    cs->head->command = parse_root_command(&ptr, 1, &err);
    if (err) {
        int line = 1;
        for (marker = buf; marker != ptr; marker++)
            line += *marker == '\n';
        fprintf(stderr, "%d: syntax error encountered\n", line);
        exit(1);
    }
    if (!cs->head->command)
        error(1, 0, "script contains no commands");
    command_node_t curNode = cs->head;
    while (1) {
        command_t cmd = parse_root_command(&ptr, 1, &err);
        if (err) {
            int line = 1;
            for (marker = buf; marker != ptr; marker++)
                line += *marker == '\n';
            fprintf(stderr, "%d: syntax error encountered\n", line);
            exit(1);
        }
        if (!cmd) break;
        curNode->next = allocate_command_node();
        curNode->next->command = cmd;
        curNode = curNode->next;
    }
    
    free(buf);
    return cs;
}

command_t
read_command_stream (command_stream_t s)
{
    command_node_t node = s->head;
    if (!node) return NULL;
    s->head = s->head->next;
    command_t cmd = node->command;
    free(node);
    return cmd;
}
