include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeset-page-flip
else
noinst_PROGRAMS = \
	modeset-page-flip
endif

modeset-page-flip_SOURCES = $(MODESET_PAGE_FILES)

modeset-page-flip_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
