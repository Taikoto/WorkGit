include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-atomic-crtc
else
noinst_PROGRAMS = \
	modeset-atomic-crtc
endif

modeset-atomic-crtc_SOURCES = $(MODESET_CRTC_FILES)

modeset-atomic-crtc_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
