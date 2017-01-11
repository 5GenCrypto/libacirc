#include "outputs.h"
#include "utils.h"

#include <stdlib.h>

static int acirc_add_outputs(acirc *c, const char **strs, size_t n)
{
    acirc_outputs_t *outputs = &c->outputs;
    for (size_t i = 0; i < n; ++i) {
        acircref ref = atoi(strs[i]);
        if (outputs->n >= outputs->_alloc) {
            outputs->buf = acirc_realloc(outputs->buf, 2 * outputs->_alloc * sizeof(acircref));
            outputs->_alloc *= 2;
        }
        outputs->buf[outputs->n++] = ref;
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

