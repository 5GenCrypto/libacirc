#include "acirc.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

static void topo_helper(int ref, acircref *topo, bool *seen, size_t *i, acirc *c)
{
    if (seen[ref])
        return;
    const struct acirc_gate_t *gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_CONST:
        break;
    case OP_ADD: case OP_SUB: case OP_MUL:
        for (size_t j = 0; j < gate->nargs; ++j) {
            topo_helper(gate->args[j], topo, seen, i, c);
        }
        break;
    case OP_ID:
        topo_helper(gate->args[0], topo, seen, i, c);
        break;
    }
    topo[(*i)++] = ref;
    seen[ref]    = true;
}

// returns the number of references in the topological order
size_t acirc_topological_order(acircref *topo, acirc *c, acircref ref)
{
    bool seen[c->_ref_alloc];
    size_t i = 0;
    memset(seen, '\0', sizeof seen);
    topo_helper(ref, topo, seen, &i, c);
    return i;
}

// dependencies fills an array with the refs to the subcircuit rooted at ref.
// deps is the target array, i is an index into it.
static void dependencies_helper(acircref *deps, bool *seen, int *i, acirc *c, int ref)
{
    if (seen[ref])
        return;
    const struct acirc_gate_t *gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_CONST:
        break;
    case OP_ADD: case OP_SUB: case OP_MUL:
        for (size_t j = 0; j < gate->nargs; ++j) {
            deps[(*i)++] = gate->args[j];
        }
        for (size_t j = 0; j < gate->nargs; ++j) {
            dependencies_helper(deps, seen, i, c, gate->args[j]);
        }
        seen[ref] = true;
        break;
    case OP_ID:
        deps[(*i)++] = gate->args[0];
        dependencies_helper(deps, seen, i, c, gate->args[0]);
        seen[ref] = true;
        break;
    }
}

static int dependencies(acircref *deps, acirc *c, int ref)
{
    bool seen[c->nrefs];
    int ndeps = 0;
    memset(seen, '\0', sizeof seen);
    dependencies_helper(deps, seen, &ndeps, c, ref);
    return ndeps;
}

acirc_topo_levels * acirc_topological_levels(acirc *c, acircref root)
{
    acirc_topo_levels *topo = acirc_malloc(sizeof(acirc_topo_levels));

    topo->levels      = acirc_malloc(c->nrefs * sizeof(acircref));
    topo->level_sizes = acirc_calloc(c->nrefs, sizeof(int));
    int *level_alloc  = acirc_calloc(c->nrefs, sizeof(int));
    acircref *topo_list = acirc_calloc(c->nrefs, sizeof(acircref));
    acircref *deps      = acirc_malloc(2 * c->nrefs * sizeof(acircref));

    size_t max_level = 0;
    acirc_topological_order(topo_list, c, root);

    for (size_t i = 0; i < c->nrefs; i++) {
        int ref = topo_list[i];
        int ndeps = dependencies(deps, c, ref);

        // find the right level for this ref
        for (size_t j = 0; j < c->nrefs; j++) {
            // if the ref has a dependency in this topological level, try the next level
            bool has_dep = any_in_array(deps, ndeps, topo->levels[j], topo->level_sizes[j]);
            if (has_dep) continue;
            // otherwise the ref belongs on this level

            // ensure space
            if (level_alloc[j] == 0) {
                level_alloc[j] = 2;
                topo->levels[j] = acirc_malloc(level_alloc[j] * sizeof(int));
            } else if (topo->level_sizes[j] + 1 >= level_alloc[j]) {
                level_alloc[j] *= 2;
                topo->levels[j] = acirc_realloc(topo->levels[j], level_alloc[j] * sizeof(int));
            }

            topo->levels[j][topo->level_sizes[j]] = ref; // push this ref
            topo->level_sizes[j] += 1;

            if (j > max_level)
                max_level = j;

            break; // we've found the right level for this ref: stop searching
        }
    }
    topo->nlevels = max_level + 1;
    free(topo_list);
    free(deps);
    free(level_alloc);
    return topo;
}

void acirc_topo_levels_destroy(acirc_topo_levels *topo)
{
    for (int i = 0; i < topo->nlevels; i++)
        free(topo->levels[i]);
    free(topo->levels);
    free(topo->level_sizes);
    free(topo);
}

