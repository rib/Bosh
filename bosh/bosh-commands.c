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

#include <gswat/gswat.h>

#include "completer.h"
#include "cli-decode.h"

#include "bosh-commands.h"
#include "bosh-main.h"

/* Chain containing all defined commands.  */
struct cmd_list_element *cmdlist;

struct cmd_list_element *infolist;

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

void
bosh_init_commands (void)
{
  struct cmd_list_element *c;

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

  bosh_add_command ("quit", class_support, bosh_quit_command, _("Exit bosh."));
  bosh_add_command_alias ("q", "quit", class_support, 1);
}

