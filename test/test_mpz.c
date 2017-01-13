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
    FILE *fp;

    acirc_verbose(true);

    fp = fopen("circuits/test_circ.acirc", "r");
    c = acirc_fread(NULL, fp);
    fclose(fp);
    if (c == NULL)
        return 1;

    result = acirc_ensure_mpz(c);

    return !result;
}
