// UCLA CS 111 Lab 1 command printing, for debugging

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>

static void
command_resource_print(command_t c, int mem_indent, int time_indent)
{
  char buf[100];
  sprintf(buf, "(peakrss: %%%dld kb, utime: %%%dld.%%03ld s)", mem_indent, time_indent-4);
  printf(buf, c->rss, c->utime/1000, c->utime%1000);
}

static void
command_indented_print (int indent, command_t c, bool measure, int mem_indent, int time_indent)
{
  switch (c->type)
    {
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
      {
	command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0], measure, mem_indent, time_indent);
    printf(" \\\n");
    if (measure) command_resource_print(c, mem_indent, time_indent);
	static char const command_label[][3] = { "&&", ";", "||", "|" };
	printf ("%*s%s\n", indent, "", command_label[c->type]);
	command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1], measure, mem_indent, time_indent);
	break;
      }

    case SIMPLE_COMMAND:
      {
    if (measure) command_resource_print(c, mem_indent, time_indent);
	char **w = c->u.word;
	printf ("%*s%s", indent, "", *w);
	while (*++w)
	  printf (" %s", *w);
	break;
      }

    case SUBSHELL_COMMAND:
      if (measure) command_resource_print(c, mem_indent, time_indent);
      printf ("%*s(\n", indent, "");
      command_indented_print (indent + 1, c->u.subshell_command, measure, mem_indent, time_indent);
      printf("\n");
      if (measure)
        printf ("%*s)", indent + mem_indent + time_indent + 25, "");
      else
        printf ("%*s)", indent, "");
      break;

    default:
      abort ();
    }

  if (c->input)
    printf ("<%s", c->input);
  if (c->output)
    printf (">%s", c->output);
}

void
print_command (command_t c, bool measure, int mem_indent, int time_indent)
{
  command_indented_print (2, c, measure, mem_indent, time_indent);
  putchar ('\n');
}
