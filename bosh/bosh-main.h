#ifndef BOSH_MAIN_H
#define BOSH_MAIN_H

G_BEGIN_DECLS

extern GSwatDebuggable *_bosh_current_debuggable;

GSwatDebuggable *
bosh_get_default_debuggable (void);

G_END_DECLS

#endif /* BOSH_MAIN_H */
