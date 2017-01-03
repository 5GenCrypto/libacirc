#include <acirc.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>

int
main(void)
{
    acirc *c;
    bool result;

    acirc_verbose(true);

    c = acirc_from_file("test_circ.acirc");
    if (c == NULL)
        return 1;

    result = acirc_ensure_mpz(c);

    return !result;
}
