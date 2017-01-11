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
    ensure_gate_space(c, ref);
    acircref *args = acirc_calloc(1, sizeof args[0]);
    args[0] = id;
    acirc_gate_t *gate = &c->gates.gates[ref];
    acirc_init_gate(gate, OP_INPUT, args, 1);
    c->ninputs++;
    return ACIRC_OK;
}

int acirc_add_const(acirc *c, acircref ref, int val)
{
    acirc_consts_t *consts = &c->consts;
    if (consts->n >= consts->_alloc) {
        consts->_alloc *= 2;
        consts->buf = acirc_realloc(consts->buf, consts->_alloc * sizeof consts->buf[0]);
    }
    consts->buf[consts->n] = val;

    ensure_gate_space(c, ref);
    acircref *args = acirc_calloc(2, sizeof args[0]);
    args[0] = consts->n;
    args[1] = val;
    acirc_gate_t *gate = &c->gates.gates[ref];
    acirc_init_gate(gate, OP_CONST, args, 2);
    consts->n++;
    return ACIRC_OK;
}

int acirc_add_gate(acirc *c, acircref ref, acirc_operation op,
                   const acircref *refs, size_t n)
{
    ensure_gate_space(c, ref);
    acircref *args = acirc_calloc(n, sizeof args[0]);
    memcpy(args, refs, n * sizeof args[0]);
    acirc_gate_t *gate = &c->gates.gates[ref];
    acirc_init_gate(gate, op, args, n);
    c->gates.n++;
    return ACIRC_OK;
}

static void * _acirc_add_extgate(acirc_extgates_t *g, acircref ref,
                                 const char *name, const acircref *refs,
                                 size_t n)
{
    for (size_t i = 0; i < g->n; ++i) {
        if (strcmp(name, g->gates[i].name) == 0) {
            return g->gates[i].build(ref, refs, n);
        }
    }
    return NULL;
}

int acirc_add_extgate(acirc *c, acircref ref, const char *name,
                      const acircref *refs, size_t n)
{
    ensure_gate_space(c, ref);
    acircref *args = acirc_calloc(n, sizeof args[0]);
    memcpy(args, refs, n * sizeof args[0]);
    acirc_gate_t *gate = &c->gates.gates[ref];
    acirc_init_gate(gate, OP_EXTERNAL, args, n);
    gate->name = strdup(name);
    gate->external = _acirc_add_extgate(&c->extgates, ref, name, refs, n);
    if (gate->external == NULL) {
        free(gate->args);
        return ACIRC_ERR;
    }
    c->gates.n++;
    return ACIRC_OK;
}
