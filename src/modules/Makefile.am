MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/include \
-I$(top_builddir)/src/include \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EFL_CFLAGS@

client_modulesdir = $(libdir)/clouseau/modules/client
client_modules_LTLIBRARIES = canvas_checker.la focus_graph_checker.la

canvas_checker_la_SOURCES = client/canvas_checker.c

canvas_checker_la_LDFLAGS = -module -avoid-version -rdynamic
canvas_checker_la_DEPENDENCIES = $(top_builddir)/config.h
canvas_checker_la_LIBADD = @EFL_LIBS@

focus_graph_checker_la_SOURCES = client/focus_graph_checker.c

focus_graph_checker_la_LDFLAGS = -module -avoid-version -rdynamic
focus_graph_checker_la_DEPENDENCIES = $(top_builddir)/config.h
focus_graph_checker_checker_la_LIBADD = @EFL_LIBS@

