#include "acirc.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool g_verbose = true;

static void ensure_gate_space (acirc *c, acircref ref);
static bool in_array (int x, int *ys, size_t len);
static bool any_in_array (acircref *xs, int xlen, int *ys, size_t ylen);
static void array_printstring_rev (int *bits, size_t n);

static void* acirc_realloc (void *ptr, size_t size);
static void* acirc_malloc  (size_t size);
static void* acirc_calloc  (size_t nmemb, size_t size);

#define max(x,y) (x >= y ? x : y)

void acirc_verbose(bool verbose)
{
    g_verbose = verbose;
}

void acirc_init(acirc *c)
{
    c->ninputs  = 0;
    c->nconsts  = 0;
    c->ngates   = 0;
    c->ntests   = 0;
    c->nrefs    = 0;
    c->noutputs = 0;
    c->_ref_alloc    = 2;
    c->_test_alloc   = 2;
    c->_outref_alloc = 2;
    c->_consts_alloc = 2;
    c->outrefs  = acirc_malloc(c->_outref_alloc * sizeof(acircref));
    c->gates    = acirc_malloc(c->_ref_alloc    * sizeof(struct acirc_args_t));
    c->testinps = acirc_malloc(c->_test_alloc   * sizeof(int*));
    c->testouts = acirc_malloc(c->_test_alloc   * sizeof(int*));
    c->consts   = acirc_malloc(c->_consts_alloc * sizeof(int));
    c->_degree_memo_allocated = false;
}

static void degree_memo_allocate(acirc *const c)
{
    c->_degree_memo   = acirc_calloc(c->ninputs+1, sizeof(size_t *));
    c->_degree_memoed = acirc_calloc(c->ninputs+1, sizeof(bool *));
    for (size_t i = 0; i < c->ninputs+1; i++) {
        c->_degree_memo[i]   = acirc_calloc(c->nrefs, sizeof(size_t));
        c->_degree_memoed[i] = acirc_calloc(c->nrefs, sizeof(bool));
    }
    c->_degree_memo_allocated = true;
}

void acirc_clear(acirc *c)
{
    for (size_t i = 0; i < c->nrefs; i++) {
        free(c->gates[i].args);
    }
    free(c->gates);
    free(c->outrefs);
    for (size_t i = 0; i < c->ntests; i++) {
        free(c->testinps[i]);
        free(c->testouts[i]);
    }
    free(c->testinps);
    free(c->testouts);
    free(c->consts);
    if (c->_degree_memo_allocated) {
        for (size_t i = 0; i < c->ninputs + 1; i++) {
            free(c->_degree_memo[i]);
            free(c->_degree_memoed[i]);
        }
        free(c->_degree_memo);
        free(c->_degree_memoed);
    }
}

extern int yyparse(acirc *);
extern FILE *yyin;
int acirc_parse(acirc *c, const char *const filename)
{
    yyin = fopen(filename, "r");
    if (yyin == NULL) {
        fprintf(stderr, "[libacirc] error: could not open file \"%s\"\n", filename);
        return ACIRC_ERR;
    }
    int ret = yyparse(c);
    fclose(yyin);
    return ret;
}

acirc * acirc_from_file(const char *const filename)
{
    acirc *c = acirc_malloc(sizeof(acirc));
    acirc_init(c);
    int ret = acirc_parse(c, filename);
    if (ret == 1) {
        acirc_clear(c);
        free(c);
        return NULL;
    }
    return c;
}

bool acirc_to_file(const acirc *const c, const char *const fname)
{
    const size_t n = c->nrefs;
    bool ret = true;
    FILE *f;

    f = fopen(fname, "w");
    if (f == NULL)
        return false;
    for (size_t i = 0; i < n; ++i) {
        const acirc_operation op = c->gates[i].op;
        switch (op) {
        case OP_INPUT:
            fprintf(f, "%ld input %ld\n", i, c->gates[i].args[0]);
            break;
        case OP_INPUT_PLAINTEXT:
            fprintf(f, "%ld input plaintext %ld\n", i, c->gates[i].args[0]);
            break;
        case OP_CONST:
            fprintf(f, "%ld const %ld\n", i, c->gates[i].args[1]);
            break;
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_ID:
            fprintf(f, "%ld %s %s", i,
                    c->gates[i].is_output ? "output" : "gate",
                    acirc_op2str(c->gates[i].op));
            for (size_t j = 0; j < c->gates[i].nargs; ++j) {
                fprintf(f, " %ld", c->gates[i].args[j]);
            }
            fprintf(f, "\n");
            break;
        }
    }
    fclose(f);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// acirc evaluation

int acirc_eval(acirc *c, acircref root, int *xs)
{
    acircref topo[c->nrefs];
    acircref vals[c->nrefs];
    const size_t n = acirc_topological_order(topo, c, root);

    for (size_t i = 0; i < n; i++) {
        const acircref ref = topo[i];
        const struct acirc_args_t *gate = &c->gates[ref];
        switch (gate->op) {
        case OP_INPUT:
            vals[ref] = xs[gate->args[0]];
            break;
        case OP_INPUT_PLAINTEXT:
            abort();            /* XXX */
        case OP_CONST:
            vals[ref] = gate->args[1];
            break;
        case OP_ADD:
            vals[ref] = 0;
            for (size_t j = 0; j < gate->nargs; ++j) {
                vals[ref] += vals[gate->args[j]];
            }
            break;
        case OP_SUB:
            assert(gate->nargs >= 1);
            vals[ref] = vals[gate->args[0]];
            for (size_t j = 1; j < gate->nargs; ++j) {
                vals[ref] -= vals[gate->args[j]];
            }
            break;
        case OP_MUL:
            vals[ref] = 1;
            for (size_t j = 0; j < gate->nargs; ++j) {
                vals[ref] *= vals[gate->args[j]];
            }
            break;
        case OP_ID:
            vals[ref] = vals[gate->args[0]];
            break;
        }
    }
    return vals[root];
}

/* #ifdef HAVE_GMP */
/* void acirc_eval_mpz_mod(mpz_t rop, acirc *const c, acircref root, mpz_t *xs, */
/*                         mpz_t *ys, const mpz_t modulus) */
/* { */
/*     /\* TODO: this should just call acirc_eval_mpz_mod_memo *\/ */
/*     const struct acirc_args_t *gate = &c->gates[root]; */
/*     const acirc_operation op = gate->op; */
/*     switch (op) { */
/*     case OP_INPUT: */
/*         mpz_set(rop, xs[gate->args[0]]); */
/*         break; */
/*     case YOP_INPUT: */
/*         mpz_set(rop, ys[gate->args[0]]); */
/*         break; */
/*     case OP_ADD: case OP_SUB: case OP_MUL: { */
/*         mpz_t zs[gate->nargs]; */
/*         for (size_t i = 0; i < gate->nargs; ++i) { */
/*             mpz_init(zs[i]); */
/*             acirc_eval_mpz_mod(zs[i], c, gate->args[i], xs, ys, modulus); */
/*         } */
/*         if (op == OP_ADD) { */
/*             mpz_set_ui(rop, 0); */
/*             for (size_t i = 0; i < gate->nargs; ++i) { */
/*                 mpz_add(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } else if (op == OP_SUB) { */
/*             mpz_set(rop, zs[0]); */
/*             for (size_t i = 1; i < gate->nargs; ++i) { */
/*                 mpz_sub(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } else if (op == OP_MUL) { */
/*             mpz_set_ui(rop, 1); */
/*             for (size_t i = 0; i < gate->nargs; ++i) { */
/*                 mpz_mul(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } */
/*         for (size_t i = 0; i < gate->nargs; ++i) { */
/*             mpz_clear(zs[i]); */
/*         } */
/*         break; */
/*     } */
/*     case OP_ID: */
/*         acirc_eval_mpz_mod(rop, c, gate->args[0], xs, ys, modulus); */
/*         break; */
/*     } */
/* } */

/* void acirc_eval_mpz_mod_memo(mpz_t rop, acirc *const c, acircref root, mpz_t *xs, */
/*                              mpz_t *ys, const mpz_t modulus, bool *known, */
/*                              mpz_t *cache) */
/* { */
/*     if (known[root]) { */
/*         mpz_set(rop, cache[root]); */
/*         return; */
/*     } */

/*     const struct acirc_args_t *gate = &c->gates[root]; */
/*     const acirc_operation op = gate->op; */
/*     switch (op) { */
/*     case XOP_INPUT: */
/*         mpz_set(rop, xs[gate->args[0]]); */
/*         break; */
/*     case YOP_INPUT: */
/*         mpz_set(rop, ys[gate->args[0]]); */
/*         break; */
/*     case OP_ADD: case OP_SUB: case OP_MUL: { */
/*         mpz_t zs[gate->nargs]; */
/*         for (size_t i = 0; i < gate->nargs; ++i) { */
/*             mpz_init(zs[i]); */
/*             acirc_eval_mpz_mod(zs[i], c, gate->args[i], xs, ys, modulus); */
/*         } */
/*         if (op == OP_ADD) { */
/*             mpz_set_ui(rop, 0); */
/*             for (size_t i = 0; i < gate->nargs; ++i) { */
/*                 mpz_add(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } else if (op == OP_SUB) { */
/*             mpz_set(rop, zs[0]); */
/*             for (size_t i = 1; i < gate->nargs; ++i) { */
/*                 mpz_sub(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } else if (op == OP_MUL) { */
/*             mpz_set_ui(rop, 1); */
/*             for (size_t i = 0; i < gate->nargs; ++i) { */
/*                 mpz_mul(rop, rop, zs[i]); */
/*                 mpz_mod(rop, rop, modulus); */
/*             } */
/*         } */
/*         for (size_t i = 0; i < gate->nargs; ++i) { */
/*             mpz_clear(zs[i]); */
/*         } */

/*         mpz_init(cache[root]); */
/*         mpz_set(cache[root], rop); */
/*         known[root] = true; */
/*         break; */
/*     } */
/*     case OP_ID: */
/*         acirc_eval_mpz_mod(rop, c, gate->args[0], xs, ys, modulus); */
/*         break; */
/*     } */
/* } */
/* #endif */

bool acirc_ensure(acirc *c)
{
    int res[c->noutputs];
    bool ok  = true;

    if (g_verbose)
        fprintf(stderr, "running acirc tests...\n");

    for (size_t test_num = 0; test_num < c->ntests; test_num++) {
        bool test_ok = true;
        for (size_t i = 0; i < c->noutputs; i++) {
            res[i] = acirc_eval(c, c->outrefs[i], c->testinps[test_num]);
            test_ok = test_ok && (res[i] == c->testouts[test_num][i]);
        }

        if (g_verbose) {
            if (!test_ok)
                printf("\033[1;41m");
            printf("test %lu input=", test_num);
            array_printstring_rev(c->testinps[test_num], c->ninputs);
            printf(" expected=");
            array_printstring_rev(c->testouts[test_num], c->noutputs);
            printf(" got=");
            array_printstring_rev(res, c->noutputs);
            if (!test_ok)
                printf("\033[0m");
            puts("");
        }

        ok = ok && test_ok;
    }
    return ok;
}

////////////////////////////////////////////////////////////////////////////////
// acirc topological ordering

static void topo_helper(int ref, acircref *topo, bool *seen, size_t *i, acirc *c)
{
    if (seen[ref])
        return;
    const struct acirc_args_t *gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_INPUT_PLAINTEXT: case OP_CONST:
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
    const struct acirc_args_t *gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_INPUT_PLAINTEXT: case OP_CONST:
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

////////////////////////////////////////////////////////////////////////////////
// acirc info calculations

static size_t acirc_depth_helper(const acirc *const c, acircref ref, size_t *memo, bool *seen)
{
    if (seen[ref])
        return memo[ref];

    const struct acirc_args_t *gate = &c->gates[ref];
    size_t ret = 0;

    switch (gate->op) {
    case OP_INPUT: case OP_INPUT_PLAINTEXT: case OP_CONST:
        ret = 0;
        break;
    case OP_ADD: case OP_SUB: case OP_MUL:
        for (size_t i = 0; i < gate->nargs; ++i) {
            size_t tmp = acirc_depth_helper(c, gate->args[i], memo, seen);
            ret = ret > tmp ? ret : tmp;
        }
        break;
    case OP_ID:
        ret = acirc_depth_helper(c, gate->args[0], memo, seen);
        break;
    default:
        abort();
    }

    seen[ref] = true;
    memo[ref] = ret;
    return ret;
}

size_t acirc_depth(const acirc *const c, acircref ref)
{
    size_t memo[c->nrefs];
    bool   seen[c->nrefs];
    memset(seen, '\0', sizeof seen);
    return acirc_depth_helper(c, ref, memo, seen);
}

static size_t acirc_degree_helper(const acirc *const c, acircref ref, size_t *const memo, bool *const seen)
{
    if (seen[ref])
        return memo[ref];

    size_t ret = 0;
    const struct acirc_args_t *gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_INPUT_PLAINTEXT: case OP_CONST:
        ret = 1;
        break;
    case OP_ADD: case OP_SUB: case OP_MUL:
        for (size_t i = 0; i < gate->nargs; ++i) {
            size_t tmp = acirc_depth_helper(c, gate->args[i], memo, seen);
            if (gate->op == OP_MUL)
                ret += tmp;
            else
                ret = ret > tmp ? ret : tmp;
        }
        break;
    case OP_ID:
        ret = acirc_degree_helper(c, gate->args[0], memo, seen);
        break;
    }

    seen[ref] = true;
    memo[ref] = ret;
    return ret;
}

size_t acirc_degree(const acirc *const c, acircref ref)
{
    size_t memo[c->nrefs];
    bool   seen[c->nrefs];
    memset(seen, '\0', sizeof seen);
    return acirc_degree_helper(c, ref, memo, seen);
}

size_t acirc_max_degree(const acirc *const c)
{
    size_t memo[c->nrefs];
    bool   seen[c->nrefs];
    memset(seen, '\0', sizeof seen);
    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; i++) {
        size_t tmp = acirc_degree_helper(c, c->outrefs[i], memo, seen);
        if (tmp > ret)
            ret = tmp;
    }

    return ret;
}

size_t acirc_var_degree(acirc *const c, acircref ref, input_id id)
{
    if (!c->_degree_memo_allocated)
        degree_memo_allocate(c);
    if (c->_degree_memoed[id][ref])
        return c->_degree_memo[id][ref];

    const struct acirc_args_t *const gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT:
        return (gate->args[0] == id) ? 1 : 0;
    case OP_CONST:
        return 0;
    case OP_ADD: case OP_SUB: case OP_MUL: {
        size_t res = 0;
        for (size_t i = 0; i < gate->nargs; ++i) {
            size_t tmp = acirc_var_degree(c, gate->args[i], id);
            if (gate->op == OP_MUL)
                res += tmp;
            else
                res = res > tmp ? res : tmp;
        }
        if (gate->op != OP_MUL) {
            c->_degree_memo[id][ref] = res;
            c->_degree_memoed[id][ref] = true;
        }
        return res;
    }
    case OP_ID: {
        size_t res = acirc_var_degree(c, gate->args[0], id);
        c->_degree_memo[id][ref] = res;
        c->_degree_memoed[id][ref] = true;
        return res;
    }
    default:
        abort();
    }
}

size_t acirc_const_degree(acirc *const c, acircref ref)
{
    if (!c->_degree_memo_allocated)
        degree_memo_allocate(c);
    if (c->_degree_memoed[c->ninputs][ref])
        return c->_degree_memo[c->ninputs][ref];

    const struct acirc_args_t *const gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT:
        return 0;
    case OP_CONST:
        return 1;
    case OP_ADD: case OP_SUB: case OP_MUL: {
        size_t res = 0;
        for (size_t i = 0; i < gate->nargs; ++i) {
            size_t tmp = acirc_const_degree(c, gate->args[i]);
            if (gate->op == OP_MUL)
                res += tmp;
            else
                res = res > tmp ? res : tmp;
        }
        if (gate->op != OP_MUL) {
            c->_degree_memo[c->ninputs][ref] = res;
            c->_degree_memoed[c->ninputs][ref] = true;
        }
        return res;
    }
    case OP_ID:
        return acirc_const_degree(c, gate->args[0]);
    default:
        abort();
    }
}

size_t acirc_max_var_degree(acirc *const c, input_id id)
{
    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; i++) {
        size_t tmp = acirc_var_degree(c, c->outrefs[i], id);
        if (tmp > ret)
            ret = tmp;
    }
    return ret;
}

size_t acirc_max_const_degree(acirc *const c)
{
    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; i++) {
        size_t tmp = acirc_const_degree(c, c->outrefs[i]);
        if (tmp > ret)
            ret = tmp;
    }
    return ret;
}

size_t acirc_delta(acirc *const c)
{
    size_t delta = acirc_max_const_degree(c);
    for (size_t i = 0; i < c->ninputs; i++) {
        delta += acirc_max_var_degree(c, i);
    }
    return delta;
}

static size_t acirc_total_degree_helper(acirc *const c, acircref ref)
{
    if (c->_degree_memoed[c->ninputs][ref])
        return c->_degree_memo[c->ninputs][ref];

    const struct acirc_args_t *const gate = &c->gates[ref];
    switch (gate->op) {
    case OP_INPUT: case OP_CONST:
        return 1;
    case OP_ADD: case OP_SUB: case OP_MUL: {
        size_t res = 0;
        for (size_t i = 0; i < gate->nargs; ++i) {
            res += acirc_total_degree_helper(c, gate->args[i]);
        }
        c->_degree_memo[c->ninputs][ref] = res;
        c->_degree_memoed[c->ninputs][ref] = true;
        return res;
    }
    case OP_ID:
        return acirc_total_degree_helper(c, gate->args[0]);
    default:
        abort();
    }
}

size_t acirc_max_total_degree(acirc *const c)
{
    if (!c->_degree_memo_allocated)
        degree_memo_allocate(c);
    /* reset memoed info */
    memset(c->_degree_memoed[c->ninputs], '\0', c->nrefs * sizeof(bool));

    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; ++i) {
        size_t tmp = acirc_total_degree_helper(c, c->outrefs[i]);
        if (tmp > ret)
            ret = tmp;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// acirc creation

void acirc_add_test(acirc *const c, const char *const inpstr,
                    const char *const outstr)
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

static void _acirc_add_input(acirc *const c, acircref ref, acircref id, bool is_plaintext)
{
    acircref *args;
    if (g_verbose) {
        fprintf(stderr, "%lu input%s %lu\n", ref, is_plaintext ? " plaintext" : "", id);
    }
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
}

void acirc_add_input(acirc *const c, acircref ref, acircref id)
{
    _acirc_add_input(c, ref, id, false);
}

void acirc_add_input_plaintext(acirc *const c, acircref ref, acircref id)
{
    _acirc_add_input(c, ref, id, true);
}

void acirc_add_const(acirc *const c, acircref ref, int val)
{
    if (g_verbose) {
        fprintf(stderr, "%lu const %d\n", ref, val);
    }
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
}

void acirc_add_gate(acirc *const c, acircref ref, acirc_operation op,
                    const acircref *const refs, size_t n)
{
    if (g_verbose) {
        fprintf(stderr, "%lu %s", ref, acirc_op2str(op));
        for (size_t i = 0; i < n; ++i) {
            fprintf(stderr, " %lu", refs[i]);
        }
        fprintf(stderr, "\n");
    }
    ensure_gate_space(c, ref);
    c->ngates += 1;
    c->nrefs += 1;
    c->gates[ref].op = op;
    acircref *args = acirc_calloc(n, sizeof(acircref));
    memcpy(args, refs, n * sizeof(acircref));
    c->gates[ref].args = args;
    c->gates[ref].nargs = n;
    c->gates[ref].is_output = false;
}

void acirc_add_output(acirc *const c, acircref ref)
{
    if (c->noutputs >= c->_outref_alloc) {
        c->outrefs = acirc_realloc(c->outrefs, 2 * c->_outref_alloc * sizeof(acircref));
        c->_outref_alloc *= 2;
    }
    c->outrefs[c->noutputs++] = ref;
    c->gates[ref].is_output = true;
}

////////////////////////////////////////////////////////////////////////////////
// helpers

static void ensure_gate_space(acirc *c, acircref ref)
{
    if (ref >= c->_ref_alloc) {
        c->gates = acirc_realloc(c->gates, 2 * c->_ref_alloc * sizeof(struct acirc_args_t));
        c->_ref_alloc *= 2;
    }
}

acirc_operation acirc_str2op(char *s)
{
    if (strcmp(s, "ADD") == 0) {
        return OP_ADD;
    } else if (strcmp(s, "SUB") == 0) {
        return OP_SUB;
    } else if (strcmp(s, "MUL") == 0) {
        return OP_MUL;
    } else if (strcmp(s, "ID") == 0) {
        return OP_ID;
    } else {
        fprintf(stderr, "unknown op '%s'\n", s);
        abort();
    }
}

char * acirc_op2str(acirc_operation op)
{
    switch (op) {
    case OP_ADD:
        return "ADD";
    case OP_SUB:
        return "SUB";
    case OP_MUL:
        return "MUL";
    case OP_ID:
        return "ID";
    default:
        return NULL;
    }
}

static bool in_array(int x, int *ys, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (x == ys[i])
            return true;
    }
    return false;
}

static bool any_in_array(acircref *xs, int xlen, int *ys, size_t ylen)
{
    for (int i = 0; i < xlen; i++) {
        if (in_array(xs[i], ys, ylen))
            return true;
    }
    return false;
}

static void array_printstring_rev(int *xs, size_t n)
{
    for (int i = n-1; i >= 0; i--)
        printf("%d", xs[i]);
}

////////////////////////////////////////////////////////////////////////////////
// custom allocators that complain when they fail

static void* acirc_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_calloc] couldn't allocate %lu bytes!\n", nmemb * size);
        abort();
    }
    return ptr;
}

static void* acirc_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_malloc] couldn't allocate %lu bytes!\n", size);
        abort();
    }
    return ptr;
}

static void* acirc_realloc(void *ptr, size_t size)
{
    void *ptr_ = realloc(ptr, size);
    if (ptr_ == NULL) {
        fprintf(stderr, "[acirc_realloc] couldn't reallocate %lu bytes!\n", size);
        abort();
    }
    return ptr_;
}
