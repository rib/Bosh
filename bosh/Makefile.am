
bin_PROGRAMS = bosh

bosh_SOURCES = cli/cli-decode.c \
	       cli/cli-setshow.c \
	       cli/cli-utils.c \
	       cli/symtab.c \
	       cli/completer.c \
	       bosh-main.c \
	       bosh-commands.c \
	       bosh-utils.c

#	       cli/cli-dump.c \
#	       cli/cli-logging.c \
#	       cli/cli-interp.c \
#	       cli/cli-script.c \
#	       cli/cli-utils.c

#bosh_LDADD =
bosh_LDFLAGS = @BOSH_DEP_LIBS@ -export-dynamic
bosh_CFLAGS = @BOSH_DEP_CFLAGS@ @EXTRA_CFLAGS@
bosh_CPPFLAGS = \
    -DSHARE_DIR=\"$(pkgdatadir)\" \
    -DBOSH_LOCALEDIR=\""$(boshlocaledir)"\" \
    -DBOSH_BIN_DIR=\""$(bindir)"\" \
    -I$(top_srcdir) \
    -I$(top_srcdir)/bosh \
    -I$(top_srcdir)/bosh/cli \
    @EXTRA_CPPFLAGS@

#EXTRA_DIST =

#@INTLTOOL_DESKTOP_RULE@

CLEANFILES =

DISTCLEANFILES = \
	$(DISTCLEANFILES)

