#include "build.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int acirc_add_command(acirc *c, const char *name, const char **strs, size_t n)
{
    for (size_t i = 0; i < c->commands.n; ++i) {
        const acirc_command_t *cmd = &c->commands.commands[i];
        if (strcmp(name, cmd->name) == 0)
            return cmd->f(c, strs, n);
    }
    fprintf(stderr, "error: unknown command '%s'\n", name);
    return ACIRC_ERR;
}

int acirc_add_input(acirc *c, acircref ref, acircref id)
{
    acircref *args;
    args = acirc_calloc(1, sizeof(acircref));
    ensure_gate_space(c, ref);
    c->nrefs++;
    c->gates[ref].op = OP_INPUT;
    c->ninputs++;
    args[0] = id;
    c->gates[ref].args = args;
    c->gates[ref].nargs = 1;
    return ACIRC_OK;
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

