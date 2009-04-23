
bin_PROGRAMS = bosh

bosh_SOURCES = bosh.c

#bosh_LDADD =
bosh_LDFLAGS = @BOSH_DEP_LIBS@ -export-dynamic
bosh_CFLAGS = @BOSH_DEP_CFLAGS@ @EXTRA_CFLAGS@
bosh_CPPFLAGS = \
    -DSHARE_DIR=\"$(pkgdatadir)\" \
    -DBOSH_LOCALEDIR=\""$(boshlocaledir)"\" \
    -DBOSH_BIN_DIR=\""$(bindir)"\" \
    -I$(top_srcdir) \
    -I$(top_srcdir)/bosh \
    @EXTRA_CPPFLAGS@

#EXTRA_DIST =

#@INTLTOOL_DESKTOP_RULE@

CLEANFILES =

DISTCLEANFILES = \
	$(DISTCLEANFILES)

