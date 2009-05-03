
#include <stdlib.h>
#include <glib.h>
#include <readline/readline.h>

#include "cli-decode.h"

#include "bosh-commands.h"

/* Prototypes for local functions.  */
static char *
line_completion_function (const char *text, int matches,
			  char *line_buffer,
			  int point);

/* readline uses the word breaks for two things:
   (1) In figuring out where to point the TEXT parameter to the
   rl_completion_entry_function.  Since we don't use TEXT for much,
   it doesn't matter a lot what the word breaks are for this purpose, but
   it does affect how much stuff M-? lists.
   (2) If one of the matches contains a word break character, readline
   will quote it.  That's why we switch between
   current_language->la_word_break_characters() and
   gdb_completer_command_word_break_characters.  I'm not sure when
   we need this behavior (perhaps for funky characters in C++ symbols?).  */

/* Variables which are necessary for fancy command line editing.  */

/* The default string containing the list of characters
   delimiting words.  This is a reasonable default value that
   most languages should be able to use.  */
static char *gdb_completer_default_word_break_characters =
" \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,-";

/* When completing on command names, we remove '-' from the list of
   word break characters, since we use it in command names.  If the
   readline library sees one in any of the current completion strings,
   it thinks that the string needs to be quoted and automatically supplies
   a leading quote.  */
static char *gdb_completer_command_word_break_characters =
" \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,";

/* When completing on file names, we remove from the list of word
   break characters any characters that are commonly used in file
   names, such as '-', '+', '~', etc.  Otherwise, readline displays
   incorrect completion candidates.  */
static char *gdb_completer_file_name_break_characters = " \t\n*|\"';:?><";

//#warning "TODO: Implement locaction_completer"
#if 0
/* These are used when completing on locations, which can mix file
   names and symbol names separated by a colon.  */
static char *gdb_completer_loc_break_characters = " \t\n*|\"';:?><,";
#endif

/* Characters that can be used to quote completion strings.  Note that we
   can't include '"' because the gdb C parser treats such quoted sequences
   as strings.  */
static char *gdb_completer_quote_characters = "'";

/* Accessor for some completer data that may interest other files.  */

char *
get_gdb_completer_quote_characters (void)
{
  return gdb_completer_quote_characters;
}


/* This can be used for functions which don't want to complete on symbols
   but don't want to complete on anything else either.  */
char **
noop_completer (char *text, char *prefix)
{
  return NULL;
}

/* Line completion interface function for readline.  */

char *
bosh_readline_line_completion_function (const char *text, int matches)
{
  return line_completion_function (text, matches, rl_line_buffer, rl_point);
}

/* Complete on filenames.  */
char **
filename_completer (char *text, char *word)
{
  int subsequent_name;
  char **return_val;
  int return_val_used;
  int return_val_alloced;

  return_val_used = 0;
  /* Small for testing.  */
  return_val_alloced = 1;
  return_val = (char **) g_malloc (return_val_alloced * sizeof (char *));

  subsequent_name = 0;
  while (1)
    {
      char *p, *q;
      p = rl_filename_completion_function (text, subsequent_name);
      if (return_val_used >= return_val_alloced)
	{
	  return_val_alloced *= 2;
	  return_val =
	    (char **) g_realloc (return_val,
				return_val_alloced * sizeof (char *));
	}
      if (p == NULL)
	{
	  return_val[return_val_used++] = p;
	  break;
	}
      /* We need to set subsequent_name to a non-zero value before the
	 continue line below, because otherwise, if the first file seen
	 by GDB is a backup file whose name ends in a `~', we will loop
	 indefinitely.  */
      subsequent_name = 1;
      /* Like emacs, don't complete on old versions.  Especially useful
         in the "source" command.  */
      if (p[strlen (p) - 1] == '~')
	{
	  g_free (p);
	  continue;
	}

      if (word == text)
	/* Return exactly p.  */
	return_val[return_val_used++] = p;
      else if (word > text)
	{
	  /* Return some portion of p.  */
	  q = g_malloc (strlen (p) + 5);
	  strcpy (q, p + (word - text));
	  return_val[return_val_used++] = q;
	  g_free (p);
	}
      else
	{
	  /* Return some of TEXT plus p.  */
	  q = g_malloc (strlen (p) + (text - word) + 5);
	  strncpy (q, word, text - word);
	  q[text - word] = '\0';
	  strcat (q, p);
	  return_val[return_val_used++] = q;
	  g_free (p);
	}
    }
#if 0
  /* There is no way to do this just long enough to affect quote inserting
     without also affecting the next completion.  This should be fixed in
     readline.  FIXME.  */
  /* Ensure that readline does the right thing
     with respect to inserting quotes.  */
  rl_completer_word_break_characters = "";
#endif
  return return_val;
}

/* Complete on command names.  Used by "help".  */
char **
command_completer (char *text, char *word)
{
  return complete_on_cmdlist (cmdlist, text, word);
}

/* Here are some useful test cases for completion.  FIXME: These should
   be put in the test suite.  They should be tested with both M-? and TAB.

   "show output-" "radix"
   "show output" "-radix"
   "p" ambiguous (commands starting with p--path, print, printf, etc.)
   "p "  ambiguous (all symbols)
   "info t foo" no completions
   "info t " no completions
   "info t" ambiguous ("info target", "info terminal", etc.)
   "info ajksdlfk" no completions
   "info ajksdlfk " no completions
   "info" " "
   "info " ambiguous (all info commands)
   "p \"a" no completions (string constant)
   "p 'a" ambiguous (all symbols starting with a)
   "p b-a" ambiguous (all symbols starting with a)
   "p b-" ambiguous (all symbols)
   "file Make" "file" (word break hard to screw up here)
   "file ../gdb.stabs/we" "ird" (needs to not break word at slash)
 */

/* Generate completions all at once.  Returns a NULL-terminated array
   of strings.  Both the array and each element are allocated with
   xmalloc.  It can also return NULL if there are no completions.

   TEXT is the caller's idea of the "word" we are looking at.

   LINE_BUFFER is available to be looked at; it contains the entire text
   of the line.  POINT is the offset in that line of the cursor.  You
   should pretend that the line ends at POINT.  */

char **
complete_line (const char *text, char *line_buffer, int point)
{
  char **list = NULL;
  char *tmp_command, *p;
  /* Pointer within tmp_command which corresponds to text.  */
  char *word;
  struct cmd_list_element *c, *result_list;

  /* Choose the default set of word break characters to break completions.
     If we later find out that we are doing completions on command strings
     (as opposed to strings supplied by the individual command completer
     functions, which can be any string) then we will switch to the
     special word break set for command strings, which leaves out the
     '-' character used in some commands.  */
  rl_completer_word_break_characters =
    gdb_completer_default_word_break_characters;
    /* current_language->la_word_break_characters(); */

  /* Decide whether to complete on a list of gdb commands or on symbols. */
  tmp_command = (char *) alloca (point + 1);
  p = tmp_command;

  strncpy (tmp_command, line_buffer, point);
  tmp_command[point] = '\0';
  /* Since text always contains some number of characters leading up
     to point, we can find the equivalent position in tmp_command
     by subtracting that many characters from the end of tmp_command.  */
  word = tmp_command + point - strlen (text);

  if (point == 0)
    {
      /* An empty line we want to consider ambiguous; that is, it
	 could be any command.  */
      c = (struct cmd_list_element *) -1;
      result_list = 0;
    }
  else
    {
      c = lookup_cmd_1 (&p, cmdlist, &result_list, 1);
    }

  /* Move p up to the next interesting thing.  */
  while (*p == ' ' || *p == '\t')
    {
      p++;
    }

  if (!c)
    {
      /* It is an unrecognized command.  So there are no
	 possible completions.  */
      list = NULL;
    }
  else if (c == (struct cmd_list_element *) -1)
    {
      char *q;

      /* lookup_cmd_1 advances p up to the first ambiguous thing, but
	 doesn't advance over that thing itself.  Do so now.  */
      q = p;
      while (*q && (isalnum (*q) || *q == '-' || *q == '_'))
	++q;
      if (q != tmp_command + point)
	{
	  /* There is something beyond the ambiguous
	     command, so there are no possible completions.  For
	     example, "info t " or "info t foo" does not complete
	     to anything, because "info t" can be "info target" or
	     "info terminal".  */
	  list = NULL;
	}
      else
	{
	  /* We're trying to complete on the command which was ambiguous.
	     This we can deal with.  */
	  if (result_list)
	    {
	      list = complete_on_cmdlist (*result_list->prefixlist, p,
					  word);
	    }
	  else
	    {
	      list = complete_on_cmdlist (cmdlist, p, word);
	    }
	  /* Ensure that readline does the right thing with respect to
	     inserting quotes.  */
	  rl_completer_word_break_characters =
	    gdb_completer_command_word_break_characters;
	}
    }
  else
    {
      /* We've recognized a full command.  */

      if (p == tmp_command + point)
	{
	  /* There is no non-whitespace in the line beyond the command.  */

	  if (p[-1] == ' ' || p[-1] == '\t')
	    {
	      /* The command is followed by whitespace; we need to complete
		 on whatever comes after command.  */
	      if (c->prefixlist)
		{
		  /* It is a prefix command; what comes after it is
		     a subcommand (e.g. "info ").  */
		  list = complete_on_cmdlist (*c->prefixlist, p, word);

		  /* Ensure that readline does the right thing
		     with respect to inserting quotes.  */
		  rl_completer_word_break_characters =
		    gdb_completer_command_word_break_characters;
		}
	      else if (c->enums)
		{
		  list = complete_on_enum (c->enums, p, word);
		  rl_completer_word_break_characters =
		    gdb_completer_command_word_break_characters;
		}
	      else
		{
		  /* It is a normal command; what comes after it is
		     completed by the command's completer function.  */
		  if (c->completer == filename_completer)
		    {
		      /* Many commands which want to complete on
			 file names accept several file names, as
			 in "run foo bar >>baz".  So we don't want
			 to complete the entire text after the
			 command, just the last word.  To this
			 end, we need to find the beginning of the
			 file name by starting at `word' and going
			 backwards.  */
		      for (p = word;
			   p > tmp_command
			     && strchr (gdb_completer_file_name_break_characters, p[-1]) == NULL;
			   p--)
			;
		      rl_completer_word_break_characters =
			gdb_completer_file_name_break_characters;
		    }
		  /* else if (c->completer == location_completer) FIXME */
		  else if (c->completer == filename_completer)
		    {
		      /* Commands which complete on locations want to
			 see the entire argument.  */
		      for (p = word;
			   p > tmp_command
			     && p[-1] != ' ' && p[-1] != '\t';
			   p--)
			;
		    }
		  list = (*c->completer) (p, word);
		}
	    }
	  else
	    {
	      /* The command is not followed by whitespace; we need to
		 complete on the command itself.  e.g. "p" which is a
		 command itself but also can complete to "print", "ptype"
		 etc.  */
	      char *q;

	      /* Find the command we are completing on.  */
	      q = p;
	      while (q > tmp_command)
		{
		  if (isalnum (q[-1]) || q[-1] == '-' || q[-1] == '_')
		    --q;
		  else
		    break;
		}

	      list = complete_on_cmdlist (result_list, q, word);

	      /* Ensure that readline does the right thing
		 with respect to inserting quotes.  */
	      rl_completer_word_break_characters =
		gdb_completer_command_word_break_characters;
	    }
	}
      else
	{
	  /* There is non-whitespace beyond the command.  */

	  if (c->prefixlist && !c->allow_unknown)
	    {
	      /* It is an unrecognized subcommand of a prefix command,
		 e.g. "info adsfkdj".  */
	      list = NULL;
	    }
	  else if (c->enums)
	    {
	      list = complete_on_enum (c->enums, p, word);
	    }
	  else
	    {
	      /* It is a normal command.  */
	      if (c->completer == filename_completer)
		{
		  /* See the commentary above about the specifics
		     of file-name completion.  */
		  for (p = word;
		       p > tmp_command
			 && strchr (gdb_completer_file_name_break_characters, p[-1]) == NULL;
		       p--)
		    ;
		  rl_completer_word_break_characters =
		    gdb_completer_file_name_break_characters;
		}
	      /* else if (c->completer == location_completer) FIXME */
	      else if (c->completer == filename_completer)
		{
		  for (p = word;
		       p > tmp_command
			 && p[-1] != ' ' && p[-1] != '\t';
		       p--)
		    ;
		}
	      list = (*c->completer) (p, word);
	    }
	}
    }

  return list;
}

/* Generate completions one by one for the completer.  Each time we are
   called return another potential completion to the caller.
   line_completion just completes on commands or passes the buck to the
   command's completer function, the stuff specific to symbol completion
   is in make_symbol_completion_list.

   TEXT is the caller's idea of the "word" we are looking at.

   MATCHES is the number of matches that have currently been collected from
   calling this completion function.  When zero, then we need to initialize,
   otherwise the initialization has already taken place and we can just
   return the next potential completion string.

   LINE_BUFFER is available to be looked at; it contains the entire text
   of the line.  POINT is the offset in that line of the cursor.  You
   should pretend that the line ends at POINT.

   Returns NULL if there are no more completions, else a pointer to a string
   which is a possible completion, it is the caller's responsibility to
   free the string.  */

static char *
line_completion_function (const char *text, int matches,
			  char *line_buffer, int point)
{
  static char **list = (char **) NULL;	/* Cache of completions.  */
  static int index;			/* Next cached completion.  */
  char *output = NULL;

  if (matches == 0)
    {
      /* The caller is beginning to accumulate a new set of completions, so
         we need to find all of them now, and cache them for returning one at
         a time on future calls.  */

      if (list)
	{
	  /* Free the storage used by LIST, but not by the strings inside.
	     This is because rl_complete_internal () frees the strings.  */
	  g_free (list);
	}
      index = 0;
      list = complete_line (text, line_buffer, point);
    }

  /* If we found a list of potential completions during initialization then
     dole them out one at a time.  The vector of completions is NULL
     terminated, so after returning the last one, return NULL (and continue
     to do so) each time we are called after that, until a new list is
     available.  */

  if (list)
    {
      output = list[index];
      if (output)
	{
	  index++;
	}
    }

#if 0
  /* Can't do this because readline hasn't yet checked the word breaks
     for figuring out whether to insert a quote.  */
  if (output == NULL)
    /* Make sure the word break characters are set back to normal for the
       next time that readline tries to complete something.  */
    rl_completer_word_break_characters =
      current_language->la_word_break_characters();
#endif

  return (output);
}

