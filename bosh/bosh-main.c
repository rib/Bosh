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
#include <gio/gio.h>

#include <gswat/gswat.h>

#include "cli-decode.h"
#include "completer.h"

#include "bosh-commands.h"
#include "bosh-utils.h"

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
}

static void
on_source_uri_change (GObject *object, GParamSpec *pspec, gpointer data)
{
#if 0
  GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE (object);
  char *uri = gswat_debuggable_get_source_uri (debuggable);
  gint line = gswatt_debuggable_get_source_line (debuggable);
  print_file_range (uri, line, line);
  g_free (uri);
#endif
}

static void
on_source_line_change (GObject *object, GParamSpec *pspec, gpointer data)
{
  GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE (object);
  char *uri = gswat_debuggable_get_source_uri (debuggable);
  gint line = gswat_debuggable_get_source_line (debuggable);

  if (uri)
    {
      bosh_utils_print_file_range (uri, line, line);
      g_free (uri);
    }

  bosh_utils_print_current_frame (debuggable);
}

static void
on_state_change (GObject *object, GParamSpec *pspec, gpointer data)
{
  GSwatDebuggable *debuggable = GSWAT_DEBUGGABLE (object);
  GSwatDebuggableState state = gswat_debuggable_get_state (debuggable);
  if (state == GSWAT_DEBUGGABLE_RUNNING)
    bosh_utils_disable_prompt ();
  else
    bosh_utils_enable_prompt ();
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
  bosh_utils_enable_prompt ();

  g_io_add_watch (input, G_IO_IN, input_available_cb, NULL);

  if (session)
    _bosh_current_debuggable = GSWAT_DEBUGGABLE (gswat_gdb_debugger_new (session));


  g_signal_connect (G_OBJECT (_bosh_current_debuggable),
                    "notify::stack",
                    G_CALLBACK (on_stack_change),
                    NULL);

  g_signal_connect (G_OBJECT (_bosh_current_debuggable),
                   "notify::source-uri",
                    G_CALLBACK (on_source_uri_change),
                    NULL);
  g_signal_connect (G_OBJECT (_bosh_current_debuggable),
                   "notify::source-line",
                    G_CALLBACK (on_source_line_change),
                    NULL);

  g_signal_connect (G_OBJECT (_bosh_current_debuggable),
                   "notify::state",
                    G_CALLBACK (on_state_change),
                    NULL);

  g_main_loop_run (loop);

  return 0;
}

