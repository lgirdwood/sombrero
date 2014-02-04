
AM_CFLAGS = $(AVX_CFLAGS) $(OPENMP_CFLAGS) -I$(top_srcdir)/include

lib_LTLIBRARIES = libsombrero.la

libsombrero_la_SOURCES = \
	convolution.c \
	image.c \
	wavelet.c \
	noise.c \
	object.c

libsombrero_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-release $(LT_RELEASE) \
	-no-undefined \
	-export-dynamic
