include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-single-buffer
else
noinst_PROGRAMS = \
	modeset-single-buffer
endif

modeset-single-buffer_SOURCES = $(MODESET_SINGLE_FILES)

modeset-single-buffer_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
