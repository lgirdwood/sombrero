noinst_HEADERS = \
	debug.h \
	bmp.h

bin_PROGRAMS = \
	smbrr-atrous \
	smbrr-structures \
	smbrr-objects \
	smbrr-reconstruct
	
smbrr_atrous_SOURCES = \
	atrous.c \
	bmp.c

smbrr_structures_SOURCES = \
	structures.c \
	bmp.c

smbrr_objects_SOURCES = \
	objects.c \
	bmp.c

smbrr_reconstruct_SOURCES = \
	reconstruct.c \
	bmp.c
	
AM_CFLAGS = -I$(top_srcdir)/include

smbrr_atrous_LDADD = \
	../src/libsombrero.la

smbrr_structures_LDADD = \
	../src/libsombrero.la

smbrr_objects_LDADD = \
	../src/libsombrero.la

smbrr_reconstruct_LDADD = \
	../src/libsombrero.la

