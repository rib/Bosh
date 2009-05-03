#ifndef COMPLETER_H
#define COMPLETER_H


char *bosh_readline_line_completion_function (const char *text,
                                              int matches);

char **noop_completer (char *, char *);

char **filename_completer (char *, char *);

char **command_completer (char *text, char *word);

#endif /* COMPLETER_H */
