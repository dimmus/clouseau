AUTOMAKE_OPTIONS     = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

if HAVE_SYSTEMD_USER_SESSION
unitsdir = $(USER_SESSION_DIR)
units_DATA = clouseaud.service clouseaud.socket
endif

EXTRA_DIST = $(units_DATA)
