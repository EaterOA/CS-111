// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  bool time_travel = false;
  char* measure_file_name = NULL;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pm:t"))
      {
      case 'p': print_tree = true; break;
      case 'm': measure_file_name = optarg; break;
      case 't': time_travel = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  int ret = 0;
  command_t command;
  if (print_tree) {
    while ((command = read_command_stream (command_stream))) {
	  printf ("# %d\n", command_number++);
	  print_command (command, false, 0, 0);
    }
  }
  else if (time_travel) {
    ret = execute_time_travel (command_stream);
  }
  else {
    ret = execute_sequential (command_stream, measure_file_name);
  }

  return ret;
}
