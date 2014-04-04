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

command_t parse_simple_command(char** c)
{
    command_t com = (command_t)checked_malloc(sizeof(struct command));
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;
    com->u.word = c;
    return com;
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
