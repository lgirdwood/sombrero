
if HAVE_SSE42
SSE4_2 = libimage_ops_sse42.la
else
SSE4_2 =
endif

if HAVE_AVX
AVX = libimage_ops_avx.la
else
AVX =
endif

if HAVE_AVX2
AVX2 = libimage_ops_avx2.la
else
AVX2 =
endif

if HAVE_FMA
FMA = libimage_ops_fma.la
else
FMA =
endif

noinst_LTLIBRARIES = \
	$(SSE4_2) \
	$(AVX) \
	$(AVX2) \
	$(FMA) \
	libimage_ops.la

libimage_ops_la_SOURCES = image_ops.c
libimage_ops_la_CFLAGS = -I$(top_srcdir)/include

libimage_ops_sse42_la_SOURCES = image_ops.c
libimage_ops_sse42_la_CFLAGS = $(SSE42_CFLAGS) -I$(top_srcdir)/include

libimage_ops_avx_la_SOURCES = image_ops.c
libimage_ops_avx_la_CFLAGS = $(AVX_CFLAGS) -I$(top_srcdir)/include

libimage_ops_avx2_la_SOURCES = image_ops.c
libimage_ops_avx2_la_CFLAGS = $(AVX2_CFLAGS) -I$(top_srcdir)/include

libimage_ops_fma_la_SOURCES = image_ops.c
libimage_ops_fma_la_CFLAGS = $(FMA_CFLAGS) -I$(top_srcdir)/include

lib_LTLIBRARIES = libsombrero.la
libsombrero_la_CFLAGS = $(OPENMP_CFLAGS) -I$(top_srcdir)/include

libsombrero_la_SOURCES = \
	convolution.c \
	image.c \
	wavelet.c \
	noise.c \
	object.c \
	reconstruct.c \
	cpu.c

libsombrero_la_LIBADD = \
	-L.libs/ \
	libimage_ops.la \
	$(SSE4_2) \
	$(AVX) \
	$(AVX2) \
	$(FMA)

libsombrero_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-no-undefined \
	-export-dynamic
