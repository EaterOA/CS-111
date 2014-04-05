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
   if(c >= 'A' && c <= 'z')
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

command_t parse_simple_command(char** c, int* err)
{
    skipspace(c);

    command_t com = allocate_command();
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;   

    int num = 0;
    size_t buf_size = 0, max_size = 128;
    size_t in_buf_size = 0, out_buf_size = 0;
    char* buf = (char*)checked_malloc(max_size * sizeof(char));
    char* in_buf = (char*)checked_malloc(max_size * sizeof(char));
    char* out_buf = (char*)checked_malloc(max_size * sizeof(char));


    for (;;)
    {
        if (buf_size == max_size) 
            buf = (char*)checked_grow_alloc(buf, &max_size);	
	char ch = **c;

        if(is_valid_token(ch))
        {
            buf[buf_size] = ch;
            buf_size++;
            (*c)++;          
        }
	else if (ch == ' ')
	{
	    buf[buf_size] = '\0';
            buf_size++;
            skipspace(c);
            num++;  
	}
	else if (ch == '<')
	{
	    if(buf[buf_size-1] != '\0')
	    {
		buf[buf_size] = '\0';
		buf_size++;
		skipspace(c);
		num++;
	    }
	    
	    while((**c) != ' ' && (**c))
	    {
		if (in_buf_size == max_size) 
            	    in_buf = (char*)checked_grow_alloc(in_buf, &max_size);
 
		if(is_valid_token((**c)))
		{
		    in_buf[in_buf_size] = (**c);
		    in_buf_size++; 
		    (*c)++;
		}
		else if((**c) == '>')
		    break;
		else
		{    
	    	    if((**c) == '|' || (**c) == '&' || (**c) == '(' ||
	       		(**c) == ')' || (**c)  == ';' || (**c) == '\n')
			break;
	    	    else
			return error_ret(err);
		}
	    }
	    com->input = in_buf;
	    skipspace(c);
	}
	else if (ch == '>')
	{
	    if(buf[buf_size-1] != '\0')
	    {
		buf[buf_size] = '\0';
		buf_size++;
		skipspace(c);
		num++;
	    }
	    
	    while((**c) != ' ' && (**c))
	    {
		if (out_buf_size == max_size) 
            	    out_buf = (char*)checked_grow_alloc(out_buf, &max_size);
 
		if(is_valid_token((**c)))
		{
		    out_buf[out_buf_size] = (**c);
		    out_buf_size++; 
		    (*c)++;
		}
		else
		{
	    	    if((**c) == '|' || (**c) == '&' || (**c) == '(' ||
	       		(**c) == ')' || (**c) == ';' || (**c) == '\n')
			break;
	    	    else
			return error_ret(err);
		}
	    }
	    com->output = out_buf;
	    skipspace(c);
	}
        else if (!ch)
        {
            num++;
            break;
        }
	else
	{
	    if(ch == '|' || ch == '&' || ch == '(' ||
	       ch == ')' || ch == ';' || ch == '\n')
	    {
		num++;
		break;
	    }
	    else
		return error_ret(err);
	}
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
    command_t link, next;
    command_t cmd = parse_simple_command(c, err);
    if (!cmd) {
        if (**c) return error_ret(err);
        return NULL;
    }
    
    stack_t oprd = stack_init();
    //stack_t optr = stack_init();
    
    stack_push(oprd, cmd);
    
    for (;;) {
        char ch = **c;
        if (isTopLevel && (!ch || ch == '\n')) return cmd;
        if (isTopLevel && ch == ')') return error_ret(err);
        if (!isTopLevel && ch == ')') return cmd;
        if (!isTopLevel && (!ch || ch == '\n')) return error_ret(err);
        
        link = allocate_command();
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
            if (!isTopLevel) return error_ret(err);
            (*c)++;
            skipspace(c);
            if (**c == '\n')
            link->type = SEQUENCE_COMMAND;
        }
        else return error_ret(err);
        
        next = parse_simple_command(c, err);
        if (!next) {
            if (**c == '(') {
                (*c)++;
                next = parse_root_command(c, 0, err);
                if (err) return NULL;
                if (!next) return error_ret(err);
            }
            else return error_ret(err);
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
    char* ptr = buf;
    int err = 0;
    cs->head = allocate_command_node();
    cs->head->command = parse_root_command(&ptr, 1, &err);
    if (!cs->head->command) error(1, 0, "line xx: syntax error encountered");;
    command_node_t curNode = cs->head;
    while (1) {
        command_t cmd = parse_root_command(&ptr, 1, &err);
        if (err) error(1, 0, "line xx: syntax error encountered");
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
