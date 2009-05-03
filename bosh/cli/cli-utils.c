/* General utility routines for GDB, the GNU debugger.

   Copyright (C) 1986, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
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

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cli-utils.h"

void
fputs_filtered (const char *linebuffer, GIOChannel *stream)
{
#if 0
  GError *error = NULL;
  gchar *less_argv[] = {
      "less",
      "-F", /* exit if text can all be shown in one screen */
      NULL
  };
  int fd_in;
  GPid pid;
  gboolean  retval;
  ssize_t remaining = strlen (linebuffer);
  const char *p = linebuffer;

  retval = g_spawn_async_with_pipes (NULL, less_argv, NULL,
                                     G_SPAWN_SEARCH_PATH,
                                     NULL, NULL,
                                     &pid,
                                     &fd_in,
                                     NULL,
                                     NULL,
                                     &error);
  if (!retval) {
      g_warning ("%s: %s", _ ("Could not run less"), error->message);
      g_free (error);
      return;
  }

  while (remaining)
    {
      ssize_t count = write (fd_in, linebuffer, strlen (linebuffer));
      if (count == -1)
        {
          if (errno == EAGAIN || errno == EINTR)
            continue;
          else
            break;
        }
      remaining -= count;
      p += count;
    }
  close (fd_in);

  while (1)
    {
      int status;
      pid_t ret = waitpid (pid, &status, 0);
      if (!(ret == -1 && errno == EINTR))
        break;
    }
#endif
  g_print ("%s", linebuffer);
}
/* Print a variable number of ARGS using format FORMAT.  If this
   information is going to put the amount written (since the last call
   to REINITIALIZE_MORE_FILTER or the last page break) over the page size,
   call prompt_for_continue to get the users permision to continue.

   Unlike fprintf, this function does not return a value.

   We implement three variants, vfprintf (takes a vararg list and stream),
   fprintf (takes a stream to write on), and printf (the usual).

   Note also that a longjmp to top level may occur in this routine
   (since prompt_for_continue may do so) so this routine should not be
   called when cleanups are not in place.  */

void
vfprintf_filtered (GIOChannel *stream, const char *format, va_list args)
{
  char *linebuffer = g_strdup_vprintf (format, args);
  fputs_filtered (linebuffer, stream);
}

void
fprintf_filtered (GIOChannel *stream, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  vfprintf_filtered (stream, format, args);
  va_end (args);
}

void
printf_filtered (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  vfprintf_filtered (NULL, format, args);
  va_end (args);
}

void
wrap_here (char *indent)
{

}

/* Make a copy of the string at PTR with SIZE characters
   (and add a null character at the end in the copy).
   Uses malloc to get the space.  Returns the address of the copy.  */

char *
savestring (const char *ptr, size_t size)
{
  char *p = (char *) g_malloc (size + 1);
  memcpy (p, ptr, size);
  p[size] = 0;
  return p;
}

long long
parse_and_eval_long (char *exp)
{
  g_warning ("FIXME: Get gswat to evaluate this as an expression");
  return 0;
}

