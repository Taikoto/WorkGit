include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-plane-test
else
noinst_PROGRAMS = \
	modeset-plane-test
endif

modeset-plane-test_SOURCES = $(MODESET_PLANE_FILES)

modeset-plane-test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
