ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

desktopdir = $(datadir)/applications
desktop_DATA = horizon.desktop

EXTRA_DIST = horizon.desktop

UPDATE_DESKTOP = update-desktop-database $(datadir)/applications || :

install-data-hook: 
	$(UPDATE_DESKTOP)
uninstall-hook: 
	$(UPDATE_DESKTOP)
distuninstallcheck:
	@:

