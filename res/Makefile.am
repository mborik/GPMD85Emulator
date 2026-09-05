# GPMD85emu automake by mborik

AUTOMAKE_OPTIONS = foreign
RES = \
	default.conf \
	icon128.bmp \
	icon32.bmp \
	icon.png

pkgdata_DATA = $(RES)

desktopdir = $(datadir)/applications
desktop_DATA = GPMD85emu.desktop

icondir = $(datadir)/pixmaps

install-data-hook:
	$(MKDIR_P) "$(DESTDIR)$(icondir)"
	$(INSTALL_DATA) "$(srcdir)/icon.png" "$(DESTDIR)$(icondir)/GPMD85emu.png"

EXTRA_DIST = $(RES) $(desktop_DATA)
