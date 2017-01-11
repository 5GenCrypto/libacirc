#include "test.h"
#include "utils.h"

#include <string.h>

static int acirc_add_test(acirc *c, const char **strs, size_t n)
{
    if (n != 2) {
        fprintf(stderr, "error: invalid number of arguments to 'test' command\n");
        return ACIRC_ERR;
    }

    acirc_tests_t *tests = c->tests;
    
    if (tests->n >= tests->_alloc) {
        tests->inps = acirc_realloc(tests->inps, 2 * tests->_alloc * sizeof(int**));
        tests->outs = acirc_realloc(tests->outs, 2 * tests->_alloc * sizeof(int**));
        tests->_alloc *= 2;
    }

    int inp_len = strlen(strs[0]);
    int out_len = strlen(strs[1]);
    int *inp = acirc_malloc(inp_len * sizeof(int));
    int *out = acirc_malloc(out_len * sizeof(int));

    for (int i = 0; i < inp_len; i++) {
        inp[i] = strs[0][inp_len - 1 - i] - 48;
    }
    for (int i = 0; i < out_len; i++) {
        out[i] = strs[1][out_len - 1 - i] - 48;
    }

    tests->inps[tests->n] = inp;
    tests->outs[tests->n] = out;
    tests->n++;
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
