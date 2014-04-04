// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include "alloc.h"

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
    int dummy;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    command_stream_t cs = (command_stream_t)checked_malloc(sizeof(struct command_stream));
    cs->dummy = (long)(get_next_byte)+(long)(get_next_byte_argument); //Remove
    return cs;
}

command_t
read_command_stream (command_stream_t s)
{
    s->dummy = 0; //Remove
    return 0;
}
