#include "build.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
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

static int acirc_add_outputs(acirc *c, const char **strs, size_t n)
{
    acirc_outputs_t *outputs = c->outputs;
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

int acirc_add_command(acirc *c, const char *cmd, const char **strs, size_t n)
{
    if (strcmp(cmd, ":test") == 0) {
        return acirc_add_test(c, strs, n);
    } else if (strcmp(cmd, ":outputs") == 0) {
        return acirc_add_outputs(c, strs, n);
    } else {
        fprintf(stderr, "error: unknown command '%s'\n", cmd);
        return ACIRC_ERR;
    }
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
    acirc_consts_t *consts = c->consts;
    if (consts->n >= consts->_alloc) {
        consts->_alloc *= 2;
        consts->buf = acirc_realloc(consts->buf, consts->_alloc * sizeof(int));
    }
    consts->buf[consts->n] = val;
    c->nrefs++;
    
    ensure_gate_space(c, ref);
    c->gates[ref].op = OP_CONST;
    acircref *args = acirc_calloc(2, sizeof(acircref));
    args[0] = consts->n++;
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
    return ACIRC_OK;
}

