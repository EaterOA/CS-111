// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include "alloc.h"

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
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
    return (command_t)checked_malloc(sizeof(struct command));
}

command_t parse_simple_command(char** c)
{
    command_t com = (command_t)checked_malloc(sizeof(struct command));
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;
   
    int num = 0; 
    char* buf = (char*)checked_malloc(512 * sizeof(char));
    int buf_size = 0;
    char* curr = *c;
    for(;;curr++)
    {
	char ch = *curr;
	if(ch != ';' && ch != '|' && ch != '&' && ch != '(' &&
	   ch != ')' && ch != '<' && ch != '>')
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
    
command_t parse_root_command(char** c, char isTopLevel)
{
    command_t cmd = parse_simple_command(c);
    command_t link, next;
    if (!cmd) return NULL;
    while (1) {
        if (isTopLevel) {
            if (!**c || **c == '\n' || **c == ';') return cmd;
            if (**c == '&') {
                (*c)++;
                if (*(*c)++ != '&') return NULL;
                link = allocate_command();
                link->type = AND_COMMAND;
            }
        }
        next = parse_simple_command(c);
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
    command_stream_t cs = (command_stream_t)checked_malloc(sizeof(struct command_stream));
    cs->head = NULL;
    int c;
    while ((c = get_next_byte(get_next_byte_argument)) != -1)
        continue;
    return cs;
}

command_t
read_command_stream (command_stream_t s)
{
    s->head = 0; //Remove
    return 0;
}
