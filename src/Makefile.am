AUTOMAKE_OPTIONS = foreign -Wall -Werror
ACLOCAL_AMFLAGS = '-Ibuild/autoconf'

AM_CFLAGS = $(COMMON_CFLAGS) $(EXTRA_CFLAGS)

lib_LTLIBRARIES=libacirc.la

PARSER = parse.y scan.l
AM_YFLAGS = -d
BUILT_SOURCES = parse.h
SOURCES =   \
acirc.c     \
build.c     \
gmp.c       \
topo.c      \
utils.c	    \
commands/outputs.c \
commands/test.c

libacirc_la_SOURCES = $(SOURCES) $(PARSER)
libacirc_la_LDFLAGS = -no-undefined -version-info 0:0:0
libacirc_la_LIBADD = -lm

include_HEADERS = acirc.h
