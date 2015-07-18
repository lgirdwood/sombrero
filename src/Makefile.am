
if HAVE_SSE42
SSE4_2 = libops_sse42.la
else
SSE4_2 =
endif

if HAVE_AVX
AVX = libops_avx.la
else
AVX =
endif

if HAVE_AVX2
AVX2 = libops_avx2.la
else
AVX2 =
endif

if HAVE_FMA
FMA = libops_fma.la
else
FMA =
endif

noinst_LTLIBRARIES = \
	$(SSE4_2) \
	$(AVX) \
	$(AVX2) \
	$(FMA) \
	libops.la

libops_la_SOURCES = \
	image_ops.c \
	convolution_2d_ops.c \
	convolution_1d_ops.c \
	signal_ops.c

libops_la_CFLAGS = \
	$(OPENMP_CFLAGS) \
	-I$(top_srcdir)/include

libops_sse42_la_SOURCES = \
	image_ops.c \
	convolution_2d_ops.c \
	convolution_1d_ops.c \
	signal_ops.c

libops_sse42_la_CFLAGS = \
	 $(OPENMP_CFLAGS) \
	 $(SSE42_CFLAGS) \
	 -I$(top_srcdir)/include

libops_avx_la_SOURCES = \
	image_ops.c \
	convolution_2d_ops.c \
	convolution_1d_ops.c \
	signal_ops.c

libops_avx_la_CFLAGS = \
	$(OPENMP_CFLAGS) \
	$(AVX_CFLAGS) \
	-I$(top_srcdir)/include

libops_avx2_la_SOURCES = \
	image_ops.c \
	convolution_2d_ops.c \
	convolution_1d_ops.c \
	signal_ops.c
	
libops_avx2_la_CFLAGS = \
	$(OPENMP_CFLAGS) \
	$(AVX2_CFLAGS) \
	-I$(top_srcdir)/include

libops_fma_la_SOURCES = \
	image_ops.c \
	convolution_2d_ops.c \
	convolution_1d_ops.c \
	signal_ops.c
	
libops_fma_la_CFLAGS = \
	$(OPENMP_CFLAGS) \
	$(FMA_CFLAGS) \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libsombrero.la
libsombrero_la_CFLAGS = \
	$(OPENMP_CFLAGS) \
	-I$(top_srcdir)/include

libsombrero_la_SOURCES = \
	convolution_1d.c \
	convolution_2d.c \
	image.c \
	wavelet_2d.c \
	wavelet_1d.c \
	noise.c \
	object.c \
	reconstruct.c \
	cpu.c \
	signal.c

libsombrero_la_LIBADD = \
	-L.libs/ \
	libops.la \
	$(SSE4_2) \
	$(AVX) \
	$(AVX2) \
	$(FMA)

libsombrero_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-no-undefined \
	-export-dynamic
