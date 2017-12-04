#include <acirc.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    bool result;
    FILE *fp;

    acirc_verbose(true);

    {
        acirc c;
        fp = fopen("test/circuits/test_circ.acirc", "r");
        acirc_init(&c);
        (void) acirc_fread(&c, fp);
        fclose(fp);

        result = acirc_ensure(&c);

        fp = fopen("test/circuits/test_circ2.acirc", "w");
        acirc_fwrite(&c, fp);
        fclose(fp);

        acirc_clear(&c);
    }

    {
        acirc *c;
        fp = fopen("test/circuits/test_circ2.acirc", "r");
        c = acirc_fread(NULL, fp);
        fclose(fp);
        if (c == NULL)
            return 1;

        result = acirc_ensure(c);

        acirc_clear(c);
        free(c);
    }

    return !result;
}
