/*
   Copyright (C) 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
   1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
   2008 Free Software Foundation, Inc.

   Copyright (C) 2009 Robert Bragg

   This file is part of Bosh

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <config.h>
#include <readline/readline.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <gswat/gswat.h>

#include "cli-decode.h"
#include "cli-setshow.h"
#include "completer.h"

#include "bosh-commands.h"

#ifdef BOSH_ENABLE_DEBUG
static const GDebugKey bosh_debug_keys[] = {
      { "misc", BOSH_DEBUG_MISC },
};

static const gint n_bosh_debug_keys = G_N_ELEMENTS (bosh_debug_keys);
#endif /* BOSH_ENABLE_DEBUG */

guint bosh_debug_flags;

static gint pid = -1;
static gchar **remaining_args = NULL;
GSwatDebuggable *_bosh_current_debuggable;


GSwatDebuggable *
bosh_get_default_debuggable (void)
{
  return _bosh_current_debuggable;
}

static void
bosh_readline_cb (char *line)
{
  struct cmd_list_element *c;
  int from_tty = 1;
  char *arg;

  /* NB: If a command is found line will be updated to point at the first
   * argument */
  c = lookup_cmd (&line, cmdlist, "", 0, 1);
  if (!c)
    return;

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
}

gboolean
input_available_cb (GIOChannel *input, GIOCondition condition, gpointer data)
{
  g_return_val_if_fail (condition == G_IO_IN, TRUE);
  rl_callback_read_char ();
  return TRUE;
}

#ifdef BOSH_ENABLE_DEBUG
static gboolean
bosh_arg_debug_cb (const char *key,
                   const char *value,
                   gpointer user_data)
{
  bosh_debug_flags |=
    g_parse_debug_string (value,
                          bosh_debug_keys,
                          n_bosh_debug_keys);
  return TRUE;
}

static gboolean
bosh_arg_no_debug_cb (const char *key,
                      const char *value,
                      gpointer user_data)
{
  bosh_debug_flags &=
    ~g_parse_debug_string (value,
                           bosh_debug_keys,
                           n_bosh_debug_keys);
  return TRUE;
}
#endif /* CLUTTER_DEBUG */

static GOptionEntry bosh_args[] = {
#ifdef BOSH_ENABLE_DEBUG
      { "bosh-debug", 0, 0, G_OPTION_ARG_CALLBACK, bosh_arg_debug_cb,
        N_("Bosh debugging flags to set"), "FLAGS" },
      { "bosh-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, bosh_arg_no_debug_cb,
        N_("Bosh debugging flags to unset"), "FLAGS" },
#endif /* BOSH_ENABLE_DEBUG */
      { "pid", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_INT, &pid,
	 "Attach to running process PIDAttach to running process PID", NULL },
      { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &remaining_args,
        "[executable-file [core-file or process-id]]" },
      { NULL, },
};

static gboolean
pre_parse_hook (GOptionContext *context,
                GOptionGroup *group,
                gpointer data,
                GError **error)
{
#ifdef BOSH_ENABLE_DEBUG
  const char *env = g_getenv ("BOSH_DEBUG");
  if (env)
    {
      bosh_debug_flags =
        g_parse_debug_string (env, bosh_debug_keys, n_bosh_debug_keys);
    }
#endif /* BOSH_ENABLE_DEBUG */

  return TRUE;
}

static GSwatSession *
parse_args (int *argc, char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *group;
  GError *error = NULL;
  GSwatSession *session = NULL;

  option_context = g_option_context_new ("[executable-file "
                                         "[core-file or process-id]]");
  //g_option_context_set_ignore_unknown_options (option_context, TRUE);
  //g_option_context_set_help_enabled (option_context, TRUE);

  group = g_option_group_new ("bosh",
                              _("Bosh Options"),
                              _("Show Bosh options"),
                              NULL, NULL);

  g_option_group_set_parse_hooks (group, pre_parse_hook, NULL);
  g_option_group_add_entries (group, bosh_args);
  //g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);

  g_option_context_set_main_group (option_context, group);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      if (error)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }
    }

  /* First we see if the user has described a new session on the command line
   */
  if (pid != -1)
    {
      gchar *target;
      if (!remaining_args)
        {
          g_message ("%s",
                     g_option_context_get_help (option_context, TRUE, NULL));
          exit (1);
        }
      session = gswat_session_new ();
      gswat_session_set_target_type (session, "PID Local");
      target = g_strdup_printf ("pid=%d file=%s", pid, remaining_args[0]);
      gswat_session_set_target (session, target);
      g_free (target);
    }
  else if (remaining_args != NULL)
    {
      int i;
      GString *target;
      session = gswat_session_new ();
      gswat_session_set_target_type (session, "Run Local");
      target = g_string_new (remaining_args[0]);
      for (i=1; remaining_args[i] != NULL; i++)
        {
          g_string_append_printf (target, " %s",
                                 g_shell_quote (remaining_args[i]));
        }
      gswat_session_set_target (session, target->str);
      g_string_free (target, TRUE);
    }

  g_option_context_free (option_context);

  return session;
}

static void
nop_log_handler (const char *log_domain,
                 GLogLevelFlags flags,
                 const char *message,
                 gpointer user_data)
{
  return;
}

/* By default g_log will output to stdout/sterr, and since we depend on
 * gswat which uses extensive logging it interferes with the command
 * line interface. */
static void
bosh_disable_g_log (void)
{
  g_log_set_default_handler (nop_log_handler, NULL);
}

static void
on_stack_change (GObject *debuggable, GParamSpec *pspec, gpointer data)
{
  static GQueue *stack = NULL;
  GList *l;
  int i;

  if (stack)
    gswat_debuggable_stack_free (stack);

  g_print ("\n");
  stack = gswat_debuggable_get_stack (GSWAT_DEBUGGABLE (debuggable));
  for(l = stack->head, i = 0; l; l = l->next, i++)
    {
      GSwatDebuggableFrame *frame;
      GSwatDebuggableFrameArgument *arg;
      GList *l2;
      GString *args = g_string_new ("");

      frame = (GSwatDebuggableFrame *)l->data;

      /* note: at this point the list is in reverse,
       * so the last in the list is our current frame
       */
      g_print ("%d) %s (", i, frame->function);

      for(l2 = frame->arguments; l2; l2 = l2->next)
        {
          arg = (GSwatDebuggableFrameArgument *)l2->data;
          g_string_append_printf (args, "%s=%s, ", arg->name, arg->value);
        }

      if(args->str[args->len - 2] == ',')
        args = g_string_truncate (args, args->len - 2);

      g_print ("%s)\n", args->str);
      g_string_free (args, TRUE);
    }
}

int
main (int argc, char **argv)
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  GIOChannel *input = g_io_channel_unix_new (STDIN_FILENO);
  const char *intro = "Bosh " PACKAGE_VERSION "\n"
    "Find out how to contribute @ http://fixme.org\n"
    "\n";
  GSwatSession *session;

  g_type_init ();
  gswat_init (&argc, &argv);

  g_print ("%s", intro);

  session = parse_args (&argc, &argv);

  bosh_disable_g_log ();
  bosh_init_commands ();

  rl_completion_entry_function = bosh_readline_line_completion_function;
  rl_callback_handler_install ("(bosh) ", bosh_readline_cb);

  g_io_add_watch (input, G_IO_IN, input_available_cb, NULL);

  if (session)
    _bosh_current_debuggable = GSWAT_DEBUGGABLE (gswat_gdb_debugger_new (session));


  g_signal_connect (G_OBJECT (_bosh_current_debuggable),
                    "notify::stack",
                    G_CALLBACK (on_stack_change),
                    NULL);

  g_main_loop_run (loop);

  return 0;
}

