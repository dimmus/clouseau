MAINTAINERCLEANFILES = Makefile.in

clouseau_start: clouseau_start.in
	$(AM_V_GEN) $(SED) -e "s|\@libdir\@|$(libdir)|" $(srcdir)/clouseau_start.in > $(builddir)/clouseau_start

bin_SCRIPTS = clouseau clouseau_start

CLEAN_FILES = clouseau_start

# FIXME: I'm not even sure .in files should be included here.
EXTRA_DIST = clouseau_start.in
