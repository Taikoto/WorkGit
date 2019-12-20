include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-double-buffer
else
noinst_PROGRAMS = \
	modeset-double-buffer
endif

modeset-double-buffer_SOURCES = $(MODESET_DOUBLE_FILES)

modeset-double-buffer_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
