AUTOMAKE_OPTIONS = foreign -Wall -Werror
AM_CFLAGS = ${DEBUG_CFLAGS} -I$(top_srcdir) -g -lgmp
AM_LDFLAGS = $(top_builddir)/src/libacirc.la

check_PROGRAMS = test

TESTS = $(check_PROGRAMS)

test_SOURCES = test.c

# all: $(TESTS)
