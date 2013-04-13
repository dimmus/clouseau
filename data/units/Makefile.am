AUTOMAKE_OPTIONS     = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

UNITS_FILES=clouseaud.service clouseaud.socket

if HAVE_SYSTEMD_USER_SESSION
unitsdir = $(USER_SESSION_DIR)
units_DATA = $(UNITS_FILES)
endif

EXTRA_DIST = $(UNITS_FILES)
