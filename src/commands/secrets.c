#include "secrets.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

static int
acirc_add_secrets(acirc *c, const char **strs, size_t n)
{
    acirc_secrets_t *s = &c->secrets;
    if (s->n != 0) {
        fprintf(stderr, "error: can only use :secrets once per circuit!\n");
        return ACIRC_ERR;
    }
    assert(s->list == NULL);
    s->n = n;
    s->list = acirc_calloc(n, sizeof s->list[0]);
    for (size_t i = 0; i < n; ++i) {
        s->list[i] = strtol(strs[i], NULL, 10);
        if (s->list[i] == LONG_MIN || s->list[i] == LONG_MAX) {
            fprintf(stderr, "'%s' not a valid wire\n", strs[i]);
            free(s);
            return ACIRC_ERR;
        }
    }
    return ACIRC_OK;
}

const acirc_command_t command_secrets = { ":secrets", acirc_add_secrets };

void acirc_add_secrets_to_file(const acirc_secrets_t *s, FILE *f)
{
    fprintf(f, ":secrets");
    for (size_t i = 0; i < s->n; ++i) {
        fprintf(f, " %ld", s->list[i]);
    }
    fprintf(f, "\n");
}
