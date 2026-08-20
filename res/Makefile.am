# GPMD85emu automake
# mborik, 10/2011

AUTOMAKE_OPTIONS = foreign
RES = \
	base.fnt \
	default.conf \
	icon.bmp \
	statusbar.bmp

if EMSCRIPTEN
bin_SCRIPTS = .emcpy
CLEANFILES = $(bin_SCRIPTS)
.emcpy:
	@echo "Copying resource files to WASM storage"
	@cp $(RES) ../storage
	@touch $@
endif

pkgdata_DATA = $(RES)

EXTRA_DIST = $(RES)
