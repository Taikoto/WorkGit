include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-atomic-plane
else
noinst_PROGRAMS = \
	modeset-atomic-plane
endif

modeset-atomic-plane_SOURCES = $(MODESET_PLANE_FILES)

modeset-atomic-plane_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
