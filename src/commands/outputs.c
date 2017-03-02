#include "outputs.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>

static int acirc_add_outputs(acirc *c, const char **strs, size_t n)
{
    acirc_outputs_t *outputs = &c->outputs;
    if (outputs->n != 0) {
        fprintf(stderr, "error: can only use :outputs once per circuit!\n");
        return ACIRC_ERR;
    }
    assert(outputs->buf == NULL);
    outputs->n = n;
    outputs->buf = acirc_calloc(n, sizeof outputs->buf[0]);
    for (size_t i = 0; i < n; ++i) {
        outputs->buf[i] = atoi(strs[i]);
    }
    return ACIRC_OK;
}
const acirc_command_t command_outputs = { ":outputs", acirc_add_outputs };

void acirc_add_outputs_to_file(const acirc_outputs_t *o, FILE *f)
{
    fprintf(f, ":outputs");
    for (size_t i = 0; i < o->n; ++i) {
        fprintf(f, " %ld", o->buf[i]);
    }
    fprintf(f, "\n");
}
