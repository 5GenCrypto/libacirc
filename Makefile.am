AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = '-Ibuild/autoconf'

AM_CFLAGS = $(COMMON_CFLAGS) $(EXTRA_CFLAGS)

lib_LTLIBRARIES=libacirc.la

PARSER = parse.y scan.l
AM_YFLAGS = -d
BUILT_SOURCES = parse.h

libacirc_la_SOURCES = acirc.c $(PARSER)
libacirc_la_LDFLAGS = -release 0.1.0 -no-undefined
libacirc_la_LIBADD = -lm

include_HEADERS = acirc.h
