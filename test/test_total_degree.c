#include <acirc.h>

#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    acirc *c;
    size_t d;

    c = acirc_from_file("circuits/test_total_degree.acirc");
    if (c == NULL)
        return 1;
    d = acirc_max_total_degree(c);
    if (d != 3) {
        fprintf(stderr, "acirc_max_total_degree failed (%d != 3)\n", d);
        return 1;
    }
    acirc_clear(c);
    free(c);
    return 0;
}
