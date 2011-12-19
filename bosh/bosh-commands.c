/* GDB CLI commands.

   Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <gswat/gswat.h>

#include "completer.h"
#include "cli-decode.h"
#include "cli-setshow.h"

#include "bosh-commands.h"
#include "bosh-main.h"
#include "bosh-utils.h"

/* Chain containing all defined commands.  */
struct cmd_list_element *cmdlist;

struct cmd_list_element *infolist;

static char *last_command = NULL;

static int list_position = -1;

/* Utility used everywhere when at least one argument is needed and
   none is supplied. */

void
bosh_command_error_no_argument (char *why)
{
  g_print (_("Argument required (%s)."), why);
}

/* Provide documentation on command or list given by COMMAND.  FROM_TTY
   is ignored.  */

static void
bosh_help_command (char *command, int from_tty)
{
  help_cmd (command, NULL);
}

/* Handle the quit command.  */
static void
bosh_quit_command (char *args, int from_tty)
{
  exit (0);
}

static void
bosh_echo_command (char *text, int from_tty)
{
  g_print ("%s", text);
}

static void
bosh_start_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  GError *error = NULL;

  if (!debuggable)
    {
      g_print ("No debugging session set up yet\n");
      return;
    }
  if (!gswat_debuggable_connect (debuggable, &error))
    {
      g_print ("Failed to connect to target: %s\n", error->message);
    }
}

static gboolean
is_debuggable_interrupted (GSwatDebuggable *debuggable, char *command)
{
  if (!debuggable)
    {
      g_print ("Ignoring %s: No debugging session has been set up yet\n",
               command);
      return FALSE;
    }
  if (gswat_debuggable_get_state (debuggable)
      != GSWAT_DEBUGGABLE_INTERRUPTED)
    {
      g_print ("Ignoring %s command while not interrupted\n", command);
      return FALSE;
    }
  return TRUE;
}

static void
bosh_next_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  if (!is_debuggable_interrupted (debuggable, "next"))
    return;
  gswat_debuggable_next (debuggable);
}

static void
bosh_step_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  if (!is_debuggable_interrupted (debuggable, "step"))
    return;
  gswat_debuggable_step (debuggable);
}

static void
bosh_finish_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  if (!is_debuggable_interrupted (debuggable, "finish"))
    return;
  gswat_debuggable_finish (debuggable);
}

static void
bosh_continue_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  if (!is_debuggable_interrupted (debuggable, "continue"))
    return;
  gswat_debuggable_continue (debuggable);
}

static void
bosh_backtrace_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  GQueue *stack = NULL;
  GList *l;
  int i;

  if (!is_debuggable_interrupted (debuggable, "backtrace"))
    return;

  stack = gswat_debuggable_get_stack (GSWAT_DEBUGGABLE (debuggable));
  for(l = stack->head, i = 0; l; l = l->next, i++)
    bosh_utils_print_frame (l->data);

  gswat_debuggable_stack_free (stack);
}

static void
bosh_frame_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  gulong frame;

  if (!is_debuggable_interrupted (debuggable, "frame"))
    return;

  list_position = -1;

  if (command)
    {
      frame = strtoul (command, NULL, 10);
      gswat_debuggable_set_frame (debuggable, frame);
    }
  else
    bosh_utils_print_current_frame (debuggable);
}

static void
bosh_list_command (char *command, int from_tty)
{
  GSwatDebuggable *debuggable = bosh_get_default_debuggable ();
  char *uri;
  int line;
  int progress_direction = 1;

  if (!is_debuggable_interrupted (debuggable, "backtrace"))
    return;

  /* XXX: We currently only support lines specified as:
   * -
   * LINE
   * FILE:LINE
   */
  if (command)
    {
      if (*command == '-')
        progress_direction = -1;
      else
        {
          char **strv = g_strsplit (command, ':', -1);
          if (strv[0] && strv[1])
            {
              uri = gswat_debuggable_get_uri_for_file (debuggable, strv[0]);
              list_position = strtoul (strv[1], &endptr, 10);
              if (endptr == strv[0])
                list_position = -1;
            }
          else
            list_position = strtoul (strv[0], &endptr, 10);
          g_strfreev (strv);
        }
    }
  else
    uri = gswat_debuggable_get_source_uri (debuggable);

  if (list_position == -1)
    line = gswat_debuggable_get_source_line (debuggable);
  else
    line = list_position;

  if (uri)
    {
      if (line >= 5)
        line -= 5;
      if (bosh_utils_print_file_range (uri, line, line + 10))
        list_position = line + 10 * progress_direction;
      g_free (uri);
    }
}

void
bosh_init_commands (void)
{
  struct cmd_list_element *c;

  bosh_add_command ("aliases", class_alias, NULL,
                    _("Aliases of other commands."));
  bosh_add_command ("files", class_files, NULL,
                    _("Specifying and examining files."));
  bosh_add_command ("breakpoints", class_breakpoint, NULL,
                    _("Making program stop at certain points."));
  bosh_add_command ("data", class_vars, NULL, _("Examining data."));
  bosh_add_command ("stack", class_stack, NULL,
                    _("Examining the stack.\n"
                      "The stack is made up of stack frames.  Gdb assigns "
                      "numbers to stack frames\n"
                      "counting from zero for the innermost (currently "
                      "executing) frame.\n\n"
                      "At any time gdb identifies one frame as the "
                      "\"selected\" frame.\n"
                      "Variable lookups are done with respect to the selected "
                      "frame.\n"
                      "When the program being debugged stops, gdb selects the "
                      "innermost frame.\n"
                      "The commands below can be used to select other frames "
                      "by number or address."));
  bosh_add_command ("running", class_run, NULL,
                    _("Running the program."));

  c = bosh_add_command ("help", class_support, bosh_help_command,
                        _("Print list of commands."));
  bosh_command_set_completer (c, command_completer);
  bosh_add_command_alias ("h", "help", class_support, 1);

  bosh_add_command ("echo", class_support, bosh_echo_command,
                    _("Print a constant string."));

#if 0
  c = bosh_add_command ("run", class_run, bosh_run_command,
                        _("Start debugged program.  You may specify "
                          "arguments to give it.\n"
                          "Args may include \"*\", or \"[...]\"; they are "
                          "expanded using \"sh\".\n"
                          "Input and output redirection with \">\", \"<\", "
                          "or \">>\" are also allowed.\n\n"
                          "With no arguments, uses arguments last specified "
                          "(with \"run\" or \"set args\").\n"
                          "To cancel previous arguments and run with no "
                          "arguments,\n"
                          "use \"set args\" without arguments."));
  bosh_command_set_completer (c, filename_completer);
  add_com_alias ("r", "run", class_run, 1);
#endif

  c = bosh_add_command ("start", class_run, bosh_start_command,
                        _("Run the debugged program until the beginning of "
                          "the main procedure.\n"
                          "You may specify arguments to give to your program,"
                          " just as with the\n"
                          "\"run\" command."));
  bosh_command_set_completer (c, filename_completer);

  bosh_add_command ("next", class_run, bosh_next_command,
                    _("Step program, proceeding through subroutine calls.\n"
                    "Like the \"step\" command as long as subroutine calls "
                    "do not happen;\n"
                    "when they do, the call is treated as one instruction.\n"
                    "Argument N means do this N times (or till program stops "
                    "for another reason)."));
  bosh_add_command_alias ("n", "next", class_run, 1);

  bosh_add_command ("step", class_run, bosh_step_command,
                    _("Step program until it reaches a different source "
                      "line.\n"
                      "Argument N means do this N times (or till program "
                      "stops for another reason)."));
  bosh_add_command_alias ("s", "step", class_run, 1);

  bosh_add_command ("finish", class_run, bosh_finish_command,
                    _("Execute until selected stack frame returns.\n"
                      "Upon return, the value returned is printed and "
                      "put in the value history."));

  bosh_add_command ("continue", class_run, bosh_continue_command,
                    _("Continue program being debugged, after signal or "
                      "breakpoint.\n"
                      "If proceeding from breakpoint, a number N may be used "
                      "as an argument,\n"
                      "which means to set the ignore count of that breakpoint "
                      "to N - 1 (so that\n"
                      "the breakpoint won't break until the Nth time it is "
                      "reached)."));
  bosh_add_command_alias ("c", "cont", class_run, 1);
  bosh_add_command_alias ("fg", "cont", class_run, 1);

  bosh_add_command ("backtrace", class_stack, bosh_backtrace_command,
                    _("Print backtrace of all stack frames, or innermost "
                      "COUNT frames.\n"
                      "With a negative argument, print outermost -COUNT "
                      "frames.\n"
                      "Use of the 'full' qualifier also prints the values "
                      "of the local variables.\n"));
  bosh_add_command_alias ("bt", "backtrace", class_stack, 0);

  bosh_add_command ("frame", class_stack, bosh_frame_command,
                    _("Select and print a stack frame.\n"
                      "With no argument, print the selected stack frame.  "
                      "(See also \"info frame\").\n"
                      "An argument specifies the frame to select.\n"
                      "It can be a stack frame number or the address of the "
                      "frame.\n"
                      "With argument, nothing is printed if input is coming "
                      "from\n"
                      "a command file or a user-defined command."));
  bosh_add_command_alias ("f", "frame", class_stack, 1);

  bosh_add_command ("list", class_files, bosh_list_command,
                    _("List specified function or line.\n"
                      "With no argument, lists ten more lines after or "
                      "around previous listing.\n"
                      "list -\" lists the ten lines before a previous "
                      "ten-line listing.\n"
                      "One argument specifies a line, and ten lines are "
                      "listed around that line.\n"
                      "Two arguments with comma between specify starting "
                      "and ending lines to list.\n"
                      "Lines can be specified in these ways:\n"
                      "  LINENUM, to list around that line in current file,\n"
                      "  FILE:LINENUM, to list around that line in that "
                      "file,\n"
                      "With two args if one is empty it stands for ten lines "
                      "away from the other arg."));
  bosh_add_command_alias ("l", "list", class_files, 1);

  bosh_add_command ("quit", class_support, bosh_quit_command, _("Exit bosh."));
  bosh_add_command_alias ("q", "quit", class_support, 1);
}

void
bosh_readline_cb (char *user_line)
{
  char *line = user_line;
  struct cmd_list_element *c;
  int from_tty = 1;
  char *arg;
  GError *error = NULL;

  bosh_utils_disable_prompt ();

  /* Repeat the last command if the user simply presses <enter>... */
  if (strcmp (line, "") == 0 && last_command)
    {
      line = last_command;
      c = bosh_lookup_command (&line, cmdlist, "", 1, &error);
    }
  else
    {
      /* Save the line for possible repeating later via <enter> later */
      if (last_command)
        g_free (last_command);
      last_command = strdup (user_line);
      c = bosh_lookup_command (&line, cmdlist, "", 1, &error);
    }

  if (!c)
    {
      bosh_utils_enable_prompt ();
      return;
    }

  /* NB: If a command is found line will be updated to point at the first
   * argument */

  /* Pass null arg rather than an empty one.  */
  arg = *line ? line : 0;

  if (c->flags & DEPRECATED_WARN_USER)
    deprecated_cmd_warning (&line);

  if (c->type == set_cmd || c->type == show_cmd)
    do_setshow_command (arg, from_tty, c);
  else if (!bosh_command_has_callback (c))
    g_print (_("That is not a command, just a help topic."));
  else
    bosh_command_call (c, arg, from_tty);

  bosh_utils_enable_prompt ();
}

