MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/include \
-I$(top_builddir)/src/include \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EFL_CFLAGS@

EXTRA_DIST = \
	     clouseau_private.h \
	     clouseau_private2.h \
	     clouseau_data_legacy.h

pkgdir = $(libdir)/clouseau
pkg_LTLIBRARIES = libclouseau_preload.la
lib_LTLIBRARIES = libclouseau.la

libclouseau_la_SOURCES = clouseau_object_information.c \
				clouseau_data_legacy.c \
				clouseau.c \
				clouseau_app.c \
				clouseau_data.c

libclouseau_la_DEPENDENCIES = $(top_builddir)/config.h
libclouseau_la_LIBADD = @EFL_LIBS@
libclouseau_la_CFLAGS = @EFL_CFLAGS@
libclouseau_la_LDFLAGS = -no-undefined @lt_enable_auto_import@ -version-info @version_info@ @release_info@

libclouseau_preload_la_SOURCES = \
			 clouseau_preload.c

libclouseau_preload_la_LDFLAGS = -module -avoid-version -rdynamic
libclouseau_preload_la_DEPENDENCIES = $(top_builddir)/config.h libclouseau.la
libclouseau_preload_la_LIBADD = libclouseau.la @EFL_LIBS@
libclouseau_preload_la_CFLAGS = @EFL_CFLAGS@

