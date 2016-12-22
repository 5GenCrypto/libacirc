AUTOMAKE_OPTIONS = foreign -Wall -Werror
AM_CFLAGS = ${DEBUG_CFLAGS} -I$(top_srcdir) -g -lgmp
AM_LDFLAGS = $(top_builddir)/src/libacirc.la

check_PROGRAMS = test_circ test_total_degree test_mpz

TESTS = $(check_PROGRAMS)

test_circ_SOURCES = test_circ.c
test_total_degree_SOURCES = test_total_degree.c
test_mpz_SOURCES = test_mpz.c

all: $(TESTS)
