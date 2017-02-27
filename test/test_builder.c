#include <acirc.h>

int main(void)
{
    acirc c;
    bool result;
    FILE *fp;

    acirc_verbose(true);
    acirc_init(&c);
    acirc_add_input(&c, 0, 0);
    acirc_add_input(&c, 1, 1);
    acirc_add_const(&c, 2, 5);
    acircref args[3] = {0, 1, 2};
    acirc_add_gate(&c, 3, OP_ADD, args, 3);
    acirc_add_output(&c, 3);
    char *test[2] = {"00", "5"};
    acirc_add_command(&c, ":test", test, 2);

    result = acirc_ensure(&c);

    fp = fopen("circuits/test_builder.acirc", "w");
    acirc_fwrite(&c, fp);
    fclose(fp);

    acirc_clear(&c);

    if (!result)
        return !result;

    fp = fopen("circuits/test_builder.acirc", "r");
    acirc_init(&c);
    acirc_fread(&c, fp);
    fclose(fp);

    result = acirc_ensure(&c);

    acirc_clear(&c);

    return !result;
}
