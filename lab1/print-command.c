// UCLA CS 111 Lab 1 command printing, for debugging

#include "command.h"
#include "command-internals.h"

#include <stdlib.h>

static void
command_resource_print(command_t c, FILE* measure_fp, int mem_indent, int time_indent)
{
  char buf[100];
  sprintf(buf, "(peakrss: %%%dld kb, cputime: %%%dld.%%03ld s)", mem_indent, time_indent-4);
  fprintf(measure_fp, buf, c->rss, c->time/1000, c->time%1000);
}

static void
command_indented_print (int indent, command_t c, FILE* measure_fp, int mem_indent, int time_indent)
{
  FILE* fp = (measure_fp ? measure_fp : stdout);
  switch (c->type)
    {
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
      {
	command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0], measure_fp, mem_indent, time_indent);
    fprintf(fp, " \\\n");
    if (measure_fp) command_resource_print(c, fp, mem_indent, time_indent);
	static char const command_label[][3] = { "&&", ";", "||", "|" };
	fprintf(fp, "%*s%s\n", indent, "", command_label[c->type]);
	command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1], measure_fp, mem_indent, time_indent);
	break;
      }

    case SIMPLE_COMMAND:
      {
    if (measure_fp) command_resource_print(c, fp, mem_indent, time_indent);
	char **w = c->u.word;
	fprintf (fp, "%*s%s", indent, "", *w);
	while (*++w)
	  fprintf (fp, " %s", *w);
	break;
      }

    case SUBSHELL_COMMAND:
      if (measure_fp) command_resource_print(c, fp, mem_indent, time_indent);
      fprintf (fp, "%*s(\n", indent, "");
      command_indented_print (indent + 1, c->u.subshell_command, measure_fp, mem_indent, time_indent);
      fprintf(fp, "\n");
      if (measure_fp)
        fprintf (fp, "%*s)", indent + mem_indent + time_indent + 27, "");
      else
        fprintf (fp, "%*s)", indent, "");
      break;

    default:
      abort ();
    }

  if (c->input)
    fprintf (fp, "<%s", c->input);
  if (c->output)
    fprintf (fp, ">%s", c->output);
}

void
print_command (command_t c, FILE* measure_fp, int mem_indent, int time_indent)
{
  command_indented_print (2, c, measure_fp, mem_indent, time_indent);
  fprintf((measure_fp ? measure_fp : stdout), "\n");
}
