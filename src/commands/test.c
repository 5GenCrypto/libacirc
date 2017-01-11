#include "test.h"
#include "utils.h"

#include <string.h>

static int acirc_add_test(acirc *c, const char **strs, size_t n)
{
    if (n != 2) {
        fprintf(stderr, "error: invalid number of arguments to 'test' command\n");
        return ACIRC_ERR;
    }

    acirc_tests_t *tests = &c->tests;
    const size_t last = tests->n++;

    tests->inps = acirc_realloc(tests->inps, tests->n * sizeof tests->inps[0]);
    tests->outs = acirc_realloc(tests->outs, tests->n * sizeof tests->outs[0]);

    const size_t inp_len = strlen(strs[0]);
    const size_t out_len = strlen(strs[1]);
    int *inp = acirc_calloc(inp_len, sizeof inp[0]);
    int *out = acirc_calloc(out_len, sizeof out[0]);

    for (size_t i = 0; i < inp_len; i++) {
        inp[i] = strs[0][inp_len - 1 - i] - 48;
    }
    for (size_t i = 0; i < out_len; i++) {
        out[i] = strs[1][out_len - 1 - i] - 48;
    }

    tests->inps[last] = inp;
    tests->outs[last] = out;
    return ACIRC_OK;
}
const acirc_command_t command_test = { ":test", acirc_add_test };

void acirc_add_tests_to_file(const acirc_tests_t *t, size_t ninputs, size_t noutputs, FILE *f)
{
    for (size_t i = 0; i < t->n; ++i) {
        fprintf(f, ":test ");
        for (size_t j = 0; j < ninputs; ++j) {
            fprintf(f, "%d", t->inps[i][j]);
        }
        fprintf(f, " ");
        for (size_t j = 0; j < noutputs; ++j) {
            fprintf(f, "%d", t->outs[i][j]);
        }
        fprintf(f, "\n");
    }
}
