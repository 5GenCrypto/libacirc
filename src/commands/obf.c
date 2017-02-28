#include "obf.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    acircref *refs;
    size_t n;
} acirc_obf_public_t;

static int acirc_add_obf_public(acirc *c, const char **strs, size_t n)
{
    acirc_obf_public_t *s = acirc_calloc(1, sizeof s[0]);
    s->refs = acirc_calloc(n, sizeof s->refs[0]);
    s->n = n;
    for (size_t i = 0; i < n; ++i) {
        s->refs[i] = atoi(strs[i]);
    }
    acirc_add_extra(&c->extras, "obf-public", s);
    return ACIRC_OK;
}

static int acirc_add_obf(acirc *c, const char **strs, size_t n)
{
    const char *cmd = strs[0];
    if (strcmp(cmd, "public") == 0) {
        if (n == 0) {
            fprintf(stderr, "error: invalid number of arguments to ':obf public' command\n");
            return ACIRC_ERR;
        }
        return acirc_add_obf_public(c, &strs[1], n - 1);
    } else {
        fprintf(stderr, "error: unknown :obf command '%s'\n", cmd);
        return ACIRC_ERR;
    }
}
const acirc_command_t command_obf = { ":obf", acirc_add_obf };


