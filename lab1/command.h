// UCLA CS 111 Lab 1 command interface

#include <stdbool.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t, bool measure, int mem_indent, int time_indent);

/* Execute the command stream in sequential mode. Set measure to true to
   measure resource consumption in script */
int execute_sequential (command_stream_t, bool);

/* Execute the command stream in time travel mode. */
int execute_time_travel (command_stream_t);
