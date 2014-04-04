// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include <error.h>
#include "alloc.h"
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

command_t parse_simple_command(char** c)
{
    command_t com = allocate_command();
    com->type = SIMPLE_COMMAND;
   
    int num = 0; 
    char* buf = (char*)checked_malloc(512 * sizeof(char));
    int buf_size = 0;
    char* curr = *c;
    for(;;curr++)
    {
	char ch = *curr;
	if(ch != ';' && ch != '|' && ch != '&' && ch != '(' &&
	   ch != ')' && ch != '<' && ch != '>' && ch != '\n')
	{
	    if(ch != ' ')
	    {
	        buf[buf_size] = ch;
	        buf_size++;
	    }
	    else
	    {
		buf[buf_size] = '\0';
		buf_size++;
		num++;
	    }
	}
	else
	{
	    num++;
	    break;
	}
    }

    if(buf_size == 0)
	return NULL;

    buf[buf_size] = '\0';
    com->u.word = (char**)checked_malloc(num * sizeof(char*));
    curr = buf;
    com->u.word[0] = curr;
    int i;
    for(i = 1; i < num; i++)
    {
	while(*curr != '\0') 
	    curr++;

	curr++;
	com->u.word[i] = curr;
    }
    return com;
}
    
command_t parse_root_command(char** c, char isTopLevel, int* err)
{
    command_t cmd = parse_simple_command(c);
    if (!cmd) return NULL;
    
    command_t link, next;
    while (1) {
        if (isTopLevel && (**c == ';' || **c == '\n')) return cmd;
        if (isTopLevel && **c == ')') return error_ret(err);
        if (!isTopLevel && **c == ')') return cmd;
        if (!isTopLevel && **c == '\n') return error_ret(err);
        
        link = allocate_command();
        if (**c == '&') {
            (*c)++;
            if (*(*c)++ != '&') return NULL;
            link->type = AND_COMMAND;
        }
        else if (**c == '|') {
            (*c)++;
            if (**c == '|') {
                (*c)++;
                link->type = OR_COMMAND;
            }
            else {
                link->type = PIPE_COMMAND;
            }
        }
        else if (**c == ';') {
            (*c)++;
            link->type = SEQUENCE_COMMAND;
        }
        else return error_ret(err);
        
        next = parse_simple_command(c);
        if (!next) {
            if (**c == '(') {
                (*c)++;
                next = parse_root_command(c, 0, err);
                if (!next) return NULL;
            }
            else return NULL;
        }
        link->u.command[0] = cmd;
        link->u.command[1] = next;
        cmd = link;
    }
    return cmd;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    //Construct command_stream
    command_stream_t cs = (command_stream_t)checked_malloc(sizeof(struct command_stream));
    
    //Read the entire script into a buf
    size_t bufsize = 0, maxsize = 512;
    char* buf = (char*)checked_malloc(maxsize * sizeof(char));
    while (1) {
        int c = get_next_byte(get_next_byte_argument);
        if (bufsize == maxsize) 
            buf = (char*)checked_grow_alloc(buf, &maxsize);
        if (c == -1) {
            buf[bufsize++] = '\0';
            break;
        }
        buf[bufsize++] = (char)c;
    }
    
    //Construct nodes from each root command
    int err = 0;
    cs->head = allocate_command_node();
    cs->head->command = parse_root_command(&buf, 1, &err);
    if (!cs->head->command) return NULL;
    command_node_t curNode = cs->head;
    while (1) {
        command_t cmd = parse_root_command(&buf, 1, &err);
        if (err) return NULL;
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
