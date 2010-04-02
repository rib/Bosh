
#include <glib.h>
#include <gio/gio.h>

#include <readline/readline.h>

#include "bosh-utils.h"
#include "bosh-commands.h"

guint prompt_disable_count = 1;

char *
bosh_utils_get_simplified_filename (GFile *file)
{
  char *cwd = g_get_current_dir();
  GFile *cwd_file = g_file_new_for_path (cwd);
  char *relative = g_file_get_relative_path (file, cwd_file);
  char *local;

  g_free (cwd);
  if (relative)
    return relative;

  local = g_file_get_path (file);
  if (local)
    return local;

  return g_file_get_uri (file);
}

void
bosh_utils_disable_prompt (void)
{
  prompt_disable_count++;
  if (prompt_disable_count == 1)
    rl_callback_handler_remove ();
}

void
bosh_utils_enable_prompt (void)
{
  prompt_disable_count--;

  g_return_if_fail (prompt_disable_count >= 0);

  if (prompt_disable_count == 0);
    rl_callback_handler_install ("(bosh) ", bosh_readline_cb);
}

void
bosh_utils_print_file_range (const char *uri, gint start, gint end)
{
  GFile *file = g_file_new_for_uri (uri);
  GFileInputStream *stream;
  GDataInputStream *data_stream;
  GError *error = NULL;
  int i;

  if (!(stream = g_file_read (file, NULL, &error)))
    {
      g_print ("Failed to open source file %s", error->message);
      g_error_free (error);
      return;
    }

  data_stream =
    g_data_input_stream_new (G_INPUT_STREAM (stream));

  g_data_input_stream_set_newline_type (data_stream,
                                        G_DATA_STREAM_NEWLINE_TYPE_ANY);

  g_print ("\n");
  for (i = 1; ; i++)
    {
      gsize len = 0;
      char *line =
        g_data_input_stream_read_line (data_stream, &len, NULL, &error);
      if (!line)
        {
          g_print ("Error reading source file %s: %s\n", uri, error->message);
          g_error_free (error);
        }
      if (i >= start && i <=end)
        g_print ("%-4d %s\n", i, line);
      else if (i > end)
        break;
    }
}

void
bosh_utils_print_frame (GSwatDebuggableFrame *frame)
{
  GSwatDebuggableFrameArgument *arg;
  GList *l;
  GString *args = g_string_new ("");
  GFile *source_file;
  char *short_filename;

  /* note: at this point the list is in reverse,
   * so the last in the list is our current frame
   */
  g_print ("%d) %s (", frame->level, frame->function);

  for(l = frame->arguments; l; l = l->next)
    {
      arg = l->data;
      g_string_append_printf (args, "%s=%s, ", arg->name, arg->value);
    }

  if(args->str[args->len - 2] == ',')
    args = g_string_truncate (args, args->len - 2);

  source_file = g_file_new_for_uri (frame->source_uri);
  short_filename = bosh_utils_get_simplified_filename (source_file);
  g_object_unref (source_file);

  g_print ("%s) %s:%d\n", args->str, short_filename, frame->line);
  g_free (short_filename);

  g_string_free (args, TRUE);
}

void
bosh_utils_print_current_frame (GSwatDebuggable *debuggable)
{
  GQueue *stack = gswat_debuggable_get_stack (debuggable);
  bosh_utils_print_frame (stack->head->data);
  gswat_debuggable_stack_free (stack);
}

