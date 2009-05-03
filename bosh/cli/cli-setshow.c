/* Handle set and show GDB commands.

   Copyright (c) 2000, 2001, 2002, 2003, 2007, 2008
   Free Software Foundation, Inc.

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

#include <ctype.h>
#include <string.h>
#include <readline/tilde.h>

#include <glib/gi18n.h>

#include "cli-decode.h"
#include "cli-setshow.h"
#include "cli-utils.h"

#include "bosh-commands.h"

/* Prototypes for local functions */

static int parse_binary_operation (char *);


static enum auto_boolean
parse_auto_binary_operation (const char *arg)
{
  if (arg != NULL && *arg != '\0')
    {
      int length = strlen (arg);
      while (isspace (arg[length - 1]) && length > 0)
	length--;
      if (strncmp (arg, "on", length) == 0
	  || strncmp (arg, "1", length) == 0
	  || strncmp (arg, "yes", length) == 0
	  || strncmp (arg, "enable", length) == 0)
	return AUTO_BOOLEAN_TRUE;
      else if (strncmp (arg, "off", length) == 0
	       || strncmp (arg, "0", length) == 0
	       || strncmp (arg, "no", length) == 0
	       || strncmp (arg, "disable", length) == 0)
	return AUTO_BOOLEAN_FALSE;
      else if (strncmp (arg, "auto", length) == 0
	       || (strncmp (arg, "-1", length) == 0 && length > 1))
	return AUTO_BOOLEAN_AUTO;
    }

  g_print (_("\"on\", \"off\" or \"auto\" expected."));
  return AUTO_BOOLEAN_INVALID;
}

static int
parse_binary_operation (char *arg)
{
  int length;

  if (!arg || !*arg)
    return 1;

  length = strlen (arg);

  while (arg[length - 1] == ' ' || arg[length - 1] == '\t')
    length--;

  if (strncmp (arg, "on", length) == 0
      || strncmp (arg, "1", length) == 0
      || strncmp (arg, "yes", length) == 0
      || strncmp (arg, "enable", length) == 0)
    return 1;
  else if (strncmp (arg, "off", length) == 0
	   || strncmp (arg, "0", length) == 0
	   || strncmp (arg, "no", length) == 0
	   || strncmp (arg, "disable", length) == 0)
    return 0;
  else
    {
      g_print (_("\"on\" or \"off\" expected."));
      return -1;
    }
}

/* Do a "set" or "show" command.  ARG is NULL if no argument, or the text
   of the argument, and FROM_TTY is nonzero if this command is being entered
   directly by the user (i.e. these are just like any other
   command).  C is the command list element for the command.  */

void
do_setshow_command (char *arg, int from_tty, struct cmd_list_element *c)
{
  if (c->type == set_cmd)
    {
      switch (c->var_type)
	{
	case var_string:
	  {
	    if (arg == NULL)
	      arg = "";
	    *(char **) c->var = g_strcompress (arg);
	  }
	  break;
	case var_string_noescape:
	  if (arg == NULL)
	    arg = "";
	  if (*(char **) c->var != NULL)
	    g_free (*(char **) c->var);
	  *(char **) c->var = savestring (arg, strlen (arg));
	  break;
	case var_optional_filename:
	  if (arg == NULL)
	    arg = "";
	  if (*(char **) c->var != NULL)
	    g_free (*(char **) c->var);
	  *(char **) c->var = savestring (arg, strlen (arg));
	  break;
	case var_filename:
	  if (arg == NULL)
	    bosh_command_error_no_argument (_("filename to set it to."));
	  if (*(char **) c->var != NULL)
	    g_free (*(char **) c->var);
	  {
	    /* Clear trailing whitespace of filename.  */
	    char *ptr = arg + strlen (arg) - 1;
	    while (ptr >= arg && (*ptr == ' ' || *ptr == '\t'))
	      ptr--;
	    *(ptr + 1) = '\0';
	  }
	  *(char **) c->var = tilde_expand (arg);
	  break;
	case var_boolean:
          {
            int val = parse_binary_operation (arg);
            if (val != -1)
              *(int *) c->var = val;
            break;
          }
	case var_auto_boolean:
          {
            enum auto_boolean val = parse_auto_binary_operation (arg);
            if (val != AUTO_BOOLEAN_INVALID)
              *(enum auto_boolean *) c->var = val;
            break;
          }
	case var_uinteger:
	  if (arg == NULL)
	    bosh_command_error_no_argument (_("integer to set it to."));
	  *(unsigned int *) c->var = parse_and_eval_long (arg);
	  if (*(unsigned int *) c->var == 0)
	    *(unsigned int *) c->var = UINT_MAX;
	  break;
	case var_integer:
	  {
	    unsigned int val;
	    if (arg == NULL)
              {
                bosh_command_error_no_argument (_("integer to set it to."));
                break;
              }
	    val = parse_and_eval_long (arg);
	    if (val == 0)
	      *(int *) c->var = INT_MAX;
	    else if (val >= INT_MAX)
              {
                g_print (_("integer %u out of range"), val);
                break;
              }
	    else
	      *(int *) c->var = val;
	    break;
	  }
	case var_zinteger:
	  if (arg == NULL)
	    bosh_command_error_no_argument (_("integer to set it to."));
	  *(int *) c->var = parse_and_eval_long (arg);
	  break;
	case var_enum:
	  {
	    int i;
	    int len;
	    int nmatches;
	    const char *match = NULL;
	    char *p;

	    /* if no argument was supplied, print an informative error message */
	    if (arg == NULL)
	      {
		char *msg;
		int msg_len = 0;
		for (i = 0; c->enums[i]; i++)
		  msg_len += strlen (c->enums[i]) + 2;

		msg = g_malloc (msg_len);
		*msg = '\0';

		for (i = 0; c->enums[i]; i++)
		  {
		    if (i != 0)
		      strcat (msg, ", ");
		    strcat (msg, c->enums[i]);
		  }
		g_print (_("Requires an argument. Valid arguments are %s."), msg);
                g_free (msg);
                break;
	      }

	    p = strchr (arg, ' ');

	    if (p)
	      len = p - arg;
	    else
	      len = strlen (arg);

	    nmatches = 0;
	    for (i = 0; c->enums[i]; i++)
	      if (strncmp (arg, c->enums[i], len) == 0)
		{
		  if (c->enums[i][len] == '\0')
		    {
		      match = c->enums[i];
		      nmatches = 1;
		      break; /* exact match. */
		    }
		  else
		    {
		      match = c->enums[i];
		      nmatches++;
		    }
		}

	    if (nmatches <= 0)
              {
	        g_print (_("Undefined item: \"%s\"."), arg);
                break;
              }

	    if (nmatches > 1)
              {
                g_print (_("Ambiguous item \"%s\"."), arg);
                break;
              }

	    *(const char **) c->var = match;
	  }
	  break;
	default:
	  g_warning (_("gdb internal error: bad var_type in do_setshow_command"));
	}
    }
  else if (c->type == show_cmd)
    {
      char *val;

      /* Possibly call the pre hook.  */
      if (c->pre_show_hook)
	(c->pre_show_hook) (c);

      switch (c->var_type)
	{
	case var_string:
          if (*(char **) c->var)
            val = g_strescape (*(char **) c->var, "");
	  break;
	case var_string_noescape:
	case var_optional_filename:
	case var_filename:
	case var_enum:
	  if (*(char **) c->var)
            val = g_strdup (*(char **) c->var);
	  break;
	case var_boolean:
          val = g_strdup (c->var ? "on" : "off");
	  break;
	case var_auto_boolean:
	  switch (*(enum auto_boolean*) c->var)
	    {
	    case AUTO_BOOLEAN_TRUE:
              val = g_strdup ("on");
	      break;
	    case AUTO_BOOLEAN_FALSE:
              val = g_strdup ("off");
	      break;
	    case AUTO_BOOLEAN_AUTO:
              val = g_strdup ("auto");
	      break;
	    default:
	      g_warning ("%s:%d %s",
                         __FILE__, __LINE__,
			 _("do_setshow_command: invalid var_auto_boolean"));
	      break;
	    }
	  break;
	case var_uinteger:
	  if (*(unsigned int *) c->var == UINT_MAX)
	    {
              val = g_strdup ("unlimited");
	      break;
	    }
	  /* else fall through */
	case var_zinteger:
          val = g_strdup_printf ("%u", *(unsigned int *) c->var);
	  break;
	case var_integer:
	  if (*(int *) c->var == INT_MAX)
            val = g_strdup ("unlimited");
	  else
            val = g_strdup_printf ("%d", *(int *) c->var);
	  break;

	default:
	  g_warning (_("gdb internal error: bad var_type in do_setshow_command"));
	}

        if (c->show_value_func != NULL)
          c->show_value_func (NULL, from_tty, c, val);
        else
          g_warning ("command %s missing show_value_func", c->name);
    }
  else
    g_warning (_("gdb internal error: bad cmd_type in do_setshow_command"));

  c->func (c, NULL, from_tty);
}

/* Show all the settings in a list of show commands.  */

void
cmd_show_list (struct cmd_list_element *list, int from_tty, char *prefix)
{
  for (; list != NULL; list = list->next)
    {
      /* If we find a prefix, run its list, prefixing our output by its
         prefix (with "show " skipped).  */
      if (list->prefixlist && !list->abbrev_flag)
	{
	  char *new_prefix = strstr (list->prefixname, "show ") + 5;
	  cmd_show_list (*list->prefixlist, from_tty, new_prefix);
	}
      else
	{
	  g_print ("%s%s:", prefix, list->name);
	  if (list->type == show_cmd)
	    do_setshow_command ((char *) NULL, from_tty, list);
	  else
	    bosh_command_call (list, NULL, from_tty);
	}
    }
}

