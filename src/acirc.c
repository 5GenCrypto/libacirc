#include "acirc.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ensure_gate_space (acirc *c, acircref ref);
static bool in_array (int x, int *ys, size_t len);
static bool any_in_array (size_t *xs, int xlen, int *ys, size_t ylen);
static void array_printstring_rev (int *bits, size_t n);

static void* acirc_realloc (void *ptr, size_t size);
static void* acirc_malloc  (size_t size);
static void* acirc_calloc  (size_t nmemb, size_t size);

#define max(x,y) (x >= y ? x : y)

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
    c->_degree_memo   = acirc_malloc((c->ninputs+1) * sizeof(size_t*));
    c->_degree_memoed = acirc_malloc((c->ninputs+1) * sizeof(bool*));
    for (size_t i = 0; i < c->ninputs+1; i++) {
        c->_degree_memo[i]   = acirc_malloc(c->nrefs * sizeof(size_t));
        c->_degree_memoed[i] = acirc_malloc(c->nrefs * sizeof(bool));
        for (size_t j = 0; j < c->nrefs; j++) {
            c->_degree_memoed[i][j] = false;
        }
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
        case XINPUT:
            fprintf(f, "%ld input x%ld\n", i, c->gates[i].args[0]);
            break;
        case YINPUT:
            fprintf(f, "%ld input x%ld %ld\n", i,
                    c->gates[i].args[0], c->gates[i].args[1]);
            break;
        case ADD:
            fprintf(f, "%ld %s ADD %ld %ld\n", i,
                    c->gates[i].is_output ? "output" : "gate",
                    c->gates[i].args[0], c->gates[i].args[1]);
            break;
        case SUB:
            fprintf(f, "%ld %s SUB %ld %ld\n", i,
                    c->gates[i].is_output ? "output" : "gate",
                    c->gates[i].args[0], c->gates[i].args[1]);
            break;
        case MUL:
            fprintf(f, "%ld %s MUL %ld %ld\n", i,
                    c->gates[i].is_output ? "output" : "gate",
                    c->gates[i].args[0], c->gates[i].args[1]);
            break;
        case ID:
            fprintf(f, "%ld %s ID %ld\n", i,
                    c->gates[i].is_output ? "output" : "gate",
                    c->gates[i].args[0]);
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
        const acirc_operation op = c->gates[ref].op;
        switch (op) {
        case XINPUT:
            vals[ref] = xs[c->gates[ref].args[0]];
            break;
        case YINPUT:
            vals[ref] = c->gates[ref].args[1];
            break;
        case ADD:
            vals[ref] = 0;
            for (size_t j = 0; j < c->gates[ref].nargs; ++j) {
                vals[ref] += vals[c->gates[ref].args[j]];
            }
            break;
        case SUB:
            vals[ref] = c->gates[ref].args[0];
            for (size_t j = 1; j < c->gates[ref].nargs; ++j) {
                vals[ref] -= vals[c->gates[ref].args[j]];
            }
            break;
        case MUL:
            vals[ref] = 1;
            for (size_t j = 0; j < c->gates[ref].nargs; ++j) {
                vals[ref] *= vals[c->gates[ref].args[j]];
            }
            break;
        case ID:
            vals[ref] = vals[c->gates[ref].args[0]];
            break;
        default:
            assert(false);
            return -1;
        }
    }
    return vals[root];
}

#ifdef HAVE_GMP
void acirc_eval_mpz_mod (mpz_t rop, acirc *c, acircref root, mpz_t *xs,
                         mpz_t *ys, mpz_t modulus)
{
    const acirc_operation op = c->gates[root].op;
    switch (op) {
    case XINPUT:
        mpz_set(rop, xs[c->gates[root].args[0]]);
        break;
    case YINPUT:
        mpz_set(rop, ys[c->gates[root].args[0]]);
        break;
    case ADD: case SUB: case MUL: {
        mpz_t xres, yres;
        mpz_inits(xres, yres, NULL);
        acirc_eval_mpz_mod(xres, c, c->gates[root].args[0], xs, ys, modulus);
        acirc_eval_mpz_mod(yres, c, c->gates[root].args[1], xs, ys, modulus);
        if (op == ADD) {
            mpz_add(rop, xres, yres);
        } else if (op == SUB) {
            mpz_sub(rop, xres, yres);
        } else if (op == MUL) {
            mpz_mul(rop, xres, yres);
        }
        mpz_mod(rop, rop, modulus);
        mpz_clears(xres, yres, NULL);
        break;
    }
    case ID:
        acirc_eval_mpz_mod(rop, c, c->gates[root].args[0], xs, ys, modulus);
        break;
    }
}

void acirc_eval_mpz_mod_memo(mpz_t rop, acirc *c, acircref root, mpz_t *xs,
                             mpz_t *ys, mpz_t modulus, bool *known, mpz_t *cache)
{
    if (known[root]) {
        mpz_set(rop, cache[root]);
        return;
    }

    const acirc_operation op = c->gates[root].op;
    switch (op) {
    case XINPUT:
        mpz_set(rop, xs[c->gates[root].args[0]]);
        break;
    case YINPUT:
        mpz_set(rop, ys[c->gates[root].args[0]]);
        break;
    case ADD: case SUB: case MUL: {
        mpz_t xres, yres;
        mpz_inits(xres, yres, NULL);
        acirc_eval_mpz_mod_memo(xres, c, c->gates[root].args[0], xs, ys, modulus, known, cache);
        acirc_eval_mpz_mod_memo(yres, c, c->gates[root].args[1], xs, ys, modulus, known, cache);
        if (op == ADD) {
            mpz_add(rop, xres, yres);
        } else if (op == SUB) {
            mpz_sub(rop, xres, yres);
        } else if (op == MUL) {
            mpz_mul(rop, xres, yres);
        }
        mpz_mod(rop, rop, modulus);
        mpz_clears(xres, yres, NULL);

        mpz_init(cache[root]);
        mpz_set(cache[root], rop);
        known[root] = true;
        break;
    }
    case ID:
        acirc_eval_mpz_mod_memo(rop, c, c->gates[root].args[0], xs, ys, modulus, known, cache);
        break;
    }
}
#endif

bool acirc_ensure(acirc *c, bool verbose)
{
    if (verbose)
        puts("running acirc tests in plaintext...");
    int *res = acirc_malloc(c->noutputs * sizeof(int));
    bool ok  = true;

    for (size_t test_num = 0; test_num < c->ntests; test_num++) {

        bool test_ok = true;

        for (size_t i = 0; i < c->noutputs; i++) {
            res[i] = acirc_eval(c, c->outrefs[i], c->testinps[test_num]);
            test_ok = test_ok && ((res[i] == 1) == (c->testouts[test_num][i] == 1));
        }

        if (verbose) {
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
    free(res);
    return ok;
}

////////////////////////////////////////////////////////////////////////////////
// acirc topological ordering

static void topo_helper(int ref, acircref *topo, bool *seen, size_t *idx, acirc *c)
{
    if (seen[ref])
        return;
    const acirc_operation op = c->gates[ref].op;
    switch (op) {
    case XINPUT: case YINPUT:
        break;
    case ADD: case SUB: case MUL:
        for (size_t i = 0; i < c->gates[ref].nargs; ++i) {
            topo_helper(c->gates[ref].args[i], topo, seen, idx, c);
        }
        break;
    case ID:
        topo_helper(c->gates[ref].args[0], topo, seen, idx, c);
        break;
    }
    topo[(*idx)++] = ref;
    seen[ref]    = 1;
}

// returns the number of references in the topological order
size_t acirc_topological_order(acircref *topo, acirc *c, acircref ref)
{
    size_t i = 0;
    bool seen[c->_ref_alloc];
    memset(seen, '\0', c->_ref_alloc * sizeof(bool));
    topo_helper(ref, topo, seen, &i, c);
    return i;
}

// dependencies fills an array with the refs to the subcircuit rooted at ref.
// deps is the target array, i is an index into it.
static void dependencies_helper(acircref *deps, bool *seen, int *i, acirc *c, int ref)
{
    if (seen[ref])
        return;
    const acirc_operation op = c->gates[ref].op;
    switch (op) {
    case XINPUT: case YINPUT:
        break;
    case ADD: case SUB: case MUL: {
        int xarg = c->gates[ref].args[0];
        int yarg = c->gates[ref].args[1];
        deps[(*i)++] = xarg;
        deps[(*i)++] = yarg;
        dependencies_helper(deps, seen, i, c, xarg);
        dependencies_helper(deps, seen, i, c, yarg);
        seen[ref] = 1;
        break;
    }
    case ID: {
        int xarg = c->gates[ref].args[0];
        deps[(*i)++] = xarg;
        dependencies_helper(deps, seen, i, c, xarg);
        seen[ref] = 1;
        break;
    }
    }
}

static int dependencies(acircref *deps, acirc *c, int ref)
{
    bool *seen = acirc_calloc(c->nrefs, sizeof(bool));
    int ndeps = 0;
    dependencies_helper(deps, seen, &ndeps, c, ref);
    free(seen);
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
    const acirc_operation op = c->gates[ref].op;
    size_t ret = 0;
    switch (op) {
    case XINPUT: case YINPUT:
        ret = 0;
        break;
    case ADD: case SUB: case MUL: {
        size_t xres = acirc_depth_helper(c, c->gates[ref].args[0], memo, seen);
        size_t yres = acirc_depth_helper(c, c->gates[ref].args[1], memo, seen);
        ret = max(xres, yres);
        break;
    }
    case ID:
        ret = acirc_depth_helper(c, c->gates[ref].args[0], memo, seen);
        break;
    }

    seen[ref] = true;
    memo[ref] = ret;
    return ret;
}

size_t acirc_depth(const acirc *const c, acircref ref)
{
    size_t memo[c->nrefs];
    bool   seen[c->nrefs];

    for (size_t i = 0; i < c->nrefs; i++)
        seen[i] = false;

    return acirc_depth_helper(c, ref, memo, seen);
}

static size_t acirc_degree_helper(const acirc *const c, acircref ref, size_t *memo, bool *seen)
{
    if (seen[ref])
        return memo[ref];

    size_t ret = 0;
    const acirc_operation op = c->gates[ref].op;
    switch (op) {
    case XINPUT: case YINPUT:
        ret = 1;
        break;
    case ADD: case SUB: case MUL: {
        size_t xres = acirc_degree_helper(c, c->gates[ref].args[0], memo, seen);
        size_t yres = acirc_degree_helper(c, c->gates[ref].args[1], memo, seen);
        if (op == MUL)
            ret = xres + yres;
        else
            ret = max(xres, yres);
        break;
    }
    case ID:
        ret = acirc_degree_helper(c, c->gates[ref].args[0], memo, seen);
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

    for (size_t i = 0; i < c->nrefs; i++)
        seen[i] = false;

    return acirc_degree_helper(c, ref, memo, seen);
}

size_t acirc_max_degree(const acirc *const c)
{
    size_t memo[c->nrefs];
    bool   seen[c->nrefs];

    for (size_t i = 0; i < c->nrefs; i++)
        seen[i] = false;

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

    const acirc_operation op = c->gates[ref].op;
    switch (op) {
    case XINPUT:
        return (c->gates[ref].args[0] == id) ? 1 : 0;
    case YINPUT:
        return 0;
    case ADD: case SUB: case MUL: {
        size_t xres = acirc_var_degree(c, c->gates[ref].args[0], id);
        size_t yres = acirc_var_degree(c, c->gates[ref].args[1], id);
        if (op == MUL)
            return xres + yres;
        size_t res = max(xres, yres); // else op == ADD || op == SUB
        c->_degree_memo[id][ref] = res;
        c->_degree_memoed[id][ref] = true;
        return res;
    }
    case ID: {
        size_t res = acirc_var_degree(c, c->gates[ref].args[0], id);
        c->_degree_memo[id][ref] = res;
        c->_degree_memoed[id][ref] = true;
        return res;
    }
    }
    assert(false);
    return 0;
}

size_t acirc_const_degree(acirc *const c, acircref ref)
{
    if (!c->_degree_memo_allocated)
        degree_memo_allocate(c);
    if (c->_degree_memoed[c->ninputs][ref])
        return c->_degree_memo[c->ninputs][ref];

    const acirc_operation op = c->gates[ref].op;
    switch (op) {
    case XINPUT:
        return 0;
    case YINPUT:
        return 1;
    case ADD: case SUB: case MUL: {
        size_t xres = acirc_const_degree(c, c->gates[ref].args[0]);
        size_t yres = acirc_const_degree(c, c->gates[ref].args[1]);
        if (op == MUL)
            return xres + yres;
        size_t res = max(xres, yres); // else op == ADD || op == SUB
        c->_degree_memo[c->ninputs][ref] = res;
        c->_degree_memoed[c->ninputs][ref] = true;
        return res;
    }
    case ID:
        return acirc_const_degree(c, c->gates[ref].args[0]);
    }
    assert(false);
    return 0;
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

// get the sum of the var degrees, maximized over the output bits
size_t acirc_delta(acirc *c)
{
    size_t delta = acirc_max_const_degree(c);
    for (size_t i = 0; i < c->ninputs; i++) {
        delta += acirc_max_var_degree(c, i);
    }
    return delta;
}

////////////////////////////////////////////////////////////////////////////////
// acirc creation

void acirc_add_test(acirc *c, char *inpstr, char *outstr)
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

    for (int i = 0; i < inp_len; i++)
        if (inpstr[inp_len-1 - i] == '1')
            inp[i] = 1;
        else
            inp[i] = 0;

    for (int i = 0; i < out_len; i++)
        if (outstr[out_len-1 - i] == '1')
            out[i] = 1;
        else
            out[i] = 0;

    c->testinps[c->ntests] = inp;
    c->testouts[c->ntests] = out;
    c->ntests += 1;
}

void acirc_add_xinput(acirc *c, acircref ref, input_id id)
{
    ensure_gate_space(c, ref);
    c->ninputs += 1;
    c->nrefs   += 1;
    c->gates[ref].op = XINPUT;
    acircref *args = acirc_calloc(1, sizeof(acircref));
    args[0] = id;
    c->gates[ref].args = args;
    c->gates[ref].nargs = 1;
}

void acirc_add_yinput(acirc *c, acircref ref, size_t id, int val)
{
    ensure_gate_space(c, ref);
    if (c->nconsts >= c->_consts_alloc) {
        c->consts = acirc_realloc(c->consts, 2 * c->_consts_alloc * sizeof(int));
        c->_consts_alloc *= 2;
    }
    c->consts[c->nconsts] = val;
    c->nconsts  += 1;
    c->nrefs    += 1;
    c->gates[ref].op  = YINPUT;
    acircref *args = acirc_calloc(2, sizeof(acircref));
    args[0] = id;
    args[1] = val;
    c->gates[ref].args = args;
    c->gates[ref].nargs = 2;
}

void acirc_add_gate(acirc *c, acircref ref, acirc_operation op, int xref, int yref, bool is_output)
{
    ensure_gate_space(c, ref);
    c->ngates   += 1;
    c->nrefs    += 1;
    c->gates[ref].op  = op;
    acircref *args = acirc_calloc(2, sizeof(acircref));
    args[0] = xref;
    args[1] = yref;
    c->gates[ref].args = args;
    c->gates[ref].nargs = 2;
    c->gates[ref].is_output = false;
    if (is_output) {
        c->gates[ref].is_output = true;
        if (c->noutputs >= c->_outref_alloc) {
            c->outrefs = acirc_realloc(c->outrefs, 2 * c->_outref_alloc * sizeof(acircref));
            c->_outref_alloc *= 2;
        }
        c->outrefs[c->noutputs++] = ref;
    }
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
        return ADD;
    } else if (strcmp(s, "SUB") == 0) {
        return SUB;
    } else if (strcmp(s, "MUL") == 0) {
        return MUL;
    } else if (strcmp(s, "ID") == 0) {
        return ID;
    } else {
        fprintf(stderr, "unknown op '%s'\n", s);
    }
    exit(EXIT_FAILURE);
}

static bool in_array(int x, int *ys, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (x == ys[i])
            return true;
    }
    return false;
}

static bool any_in_array(size_t *xs, int xlen, int *ys, size_t ylen) {
    for (int i = 0; i < xlen; i++) {
        if (in_array(xs[i], ys, ylen))
            return true;
    }
    return false;
}

static void array_printstring_rev(int *xs, size_t n)
{
    for (int i = n-1; i >= 0; i--)
        printf("%d", xs[i] == 1);
}

////////////////////////////////////////////////////////////////////////////////
// custom allocators that complain when they fail

static void* acirc_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_calloc] couldn't allocate %lu bytes!\n", nmemb * size);
        assert(false);
    }
    return ptr;
}

static void* acirc_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_malloc] couldn't allocate %lu bytes!\n", size);
        assert(false);
    }
    return ptr;
}

static void* acirc_realloc(void *ptr, size_t size)
{
    void *ptr_ = realloc(ptr, size);
    if (ptr_ == NULL) {
        fprintf(stderr, "[acirc_realloc] couldn't reallocate %lu bytes!\n", size);
        assert(false);
    }
    return ptr_;
}
