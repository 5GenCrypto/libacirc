#include "build.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void acirc_add_test(acirc *c, const char *inpstr, const char *outstr)
{
    if (c->ntests >= c->_test_alloc) {
        c->testinps = acirc_realloc(c->testinps, 2 * c->_test_alloc * sizeof(int**));
        c->testouts = acirc_realloc(c->testouts, 2 * c->_test_alloc * sizeof(int**));
        c->_test_alloc *= 2;
    }

    int inp_len = strlen(inpstr);
    int out_len = strlen(outstr);
    int *inp = acirc_malloc(inp_len * sizeof(int));
    int *out = acirc_malloc(out_len * sizeof(int));

    for (int i = 0; i < inp_len; i++) {
        inp[i] = inpstr[inp_len - 1 - i] - 48;
    }
    for (int i = 0; i < out_len; i++) {
        out[i] = outstr[out_len - 1 - i] - 48;
    }

    c->testinps[c->ntests] = inp;
    c->testouts[c->ntests] = out;
    c->ntests += 1;
}

static void acirc_add_output(acirc *c, acircref ref)
{
    if (c->noutputs >= c->_outref_alloc) {
        c->outrefs = acirc_realloc(c->outrefs, 2 * c->_outref_alloc * sizeof(acircref));
        c->_outref_alloc *= 2;
    }
    c->outrefs[c->noutputs++] = ref;
    c->gates[ref].is_output = true;
}

int acirc_add_command(acirc *c, const char *cmd, const char **strs, size_t n)
{
    if (strcmp(cmd, ":test") == 0) {
        if (n != 2) {
            fprintf(stderr, "error: invalid number of arguments to 'test' command");
            return ACIRC_ERR;
        }
        acirc_add_test(c, strs[0], strs[1]);
    } else if (strcmp(cmd, ":outputs") == 0) {
        for (size_t i = 0; i < n; ++i) {
            acirc_add_output(c, atoi(strs[i]));
        }
    } else {
        fprintf(stderr, "error: unknown command '%s'\n", cmd);
        return ACIRC_ERR;
    }
    return ACIRC_OK;
}

static int _acirc_add_input(acirc *c, acircref ref, acircref id, bool is_plaintext)
{
    acircref *args;
    args = acirc_calloc(1, sizeof(acircref));
    ensure_gate_space(c, ref);
    c->nrefs++;
    if (is_plaintext) {
        c->gates[ref].op = OP_INPUT_PLAINTEXT;
        c->npinputs++;
        args[0] = id;
    } else {
        c->gates[ref].op = OP_INPUT;
        c->ninputs++;
        args[0] = id;
    }
    c->gates[ref].args = args;
    c->gates[ref].nargs = 1;
    return ACIRC_OK;
}

int acirc_add_input(acirc *c, acircref ref, acircref id)
{
    return _acirc_add_input(c, ref, id, false);
}

int acirc_add_plaintext(acirc *c, acircref ref, acircref id)
{
    return _acirc_add_input(c, ref, id, true);
}

int acirc_add_const(acirc *c, acircref ref, int val)
{
    ensure_gate_space(c, ref);
    if (c->nconsts >= c->_consts_alloc) {
        c->consts = acirc_realloc(c->consts, 2 * c->_consts_alloc * sizeof(int));
        c->_consts_alloc *= 2;
    }
    c->consts[c->nconsts] = val;
    c->nrefs++;
    c->gates[ref].op = OP_CONST;
    acircref *args = acirc_calloc(2, sizeof(acircref));
    args[0] = c->nconsts++;
    args[1] = val;
    c->gates[ref].args = args;
    c->gates[ref].nargs = 2;
    return ACIRC_OK;
}

int acirc_add_gate(acirc *c, acircref ref, acirc_operation op,
                   const acircref *refs, size_t n)
{
    ensure_gate_space(c, ref);
    c->ngates += 1;
    c->nrefs += 1;
    c->gates[ref].op = op;
    acircref *args = acirc_calloc(n, sizeof(acircref));
    memcpy(args, refs, n * sizeof(acircref));
    c->gates[ref].args = args;
    c->gates[ref].nargs = n;
    c->gates[ref].is_output = false;
    return ACIRC_OK;
}

