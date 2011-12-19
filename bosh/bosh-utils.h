#ifndef BOSH_UTILS_H
#define BOSH_UTILS_H

#include <gswat/gswat.h>
#include <gio/gio.h>

G_BEGIN_DECLS

char *bosh_utils_get_simplified_filename (GFile *file);

void bosh_utils_enable_prompt (void);
void bosh_utils_disable_prompt (void);

gboolean bosh_utils_print_file_range (const char *uri, gint start, gint end);

void bosh_utils_print_frame (GSwatDebuggableFrame *frame);
void bosh_utils_print_current_frame (GSwatDebuggable *debuggable);

G_END_DECLS

#endif /* BOSH_UTILS_H */

