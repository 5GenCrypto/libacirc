AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = '-Ibuild/autoconf'

AM_CFLAGS = $(COMMON_CFLAGS) $(EXTRA_CFLAGS) -g

lib_LTLIBRARIES=libacirc.la

PARSER = parse.y scan.l
AM_YFLAGS = -d
BUILT_SOURCES = parse.h

libacirc_la_SOURCES = acirc.c $(PARSER)
libacirc_la_LDFLAGS = -no-undefined -version-info 0:0:0
libacirc_la_LIBADD = -lm

include_HEADERS = acirc.h
