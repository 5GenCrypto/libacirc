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

void acirc_init (acirc *c)
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
    c->args     = acirc_malloc(c->_ref_alloc    * sizeof(acircref*));
    c->ops      = acirc_malloc(c->_ref_alloc    * sizeof(acirc_operation));
    c->testinps = acirc_malloc(c->_test_alloc   * sizeof(int*));
    c->testouts = acirc_malloc(c->_test_alloc   * sizeof(int*));
    c->consts   = acirc_malloc(c->_consts_alloc * sizeof(int));
}

void acirc_clear (acirc *c)
{
    for (size_t i = 0; i < c->nrefs; i++) {
        free(c->args[i]);
    }
    free(c->args);
    free(c->ops);
    free(c->outrefs);
    for (size_t i = 0; i < c->ntests; i++) {
        free(c->testinps[i]);
        free(c->testouts[i]);
    }
    free(c->testinps);
    free(c->testouts);
    free(c->consts);
}

void acirc_destroy (acirc *c)
{
    acirc_clear(c);
    free(c);
}

extern int yyparse();
extern FILE *yyin;

void acirc_parse (acirc *c, char *filename)
{
    yyin = fopen(filename, "r");
    if (yyin == NULL) {
        fprintf(stderr, "[libacirc] error: could not open file \"%s\"\n", filename);
        exit(1);
    }
    yyparse(c);
    fclose(yyin);
}

acirc* acirc_from_file (char *filename)
{
    acirc *c = acirc_malloc(sizeof(acirc));
    acirc_init(c);
    acirc_parse(c, filename);
    return c;
}

////////////////////////////////////////////////////////////////////////////////
// acirc evaluation

int acirc_eval (acirc *c, acircref ref, int *xs)
{
    acirc_operation op = c->ops[ref];
    switch (op) {
        case XINPUT: return xs[c->args[ref][0]];
        case YINPUT: return c->args[ref][1];
        default:; // silence warning
    }
    int xres = acirc_eval(c, c->args[ref][0], xs);
    int yres = acirc_eval(c, c->args[ref][1], xs);
    switch (op) {
        case ADD: return xres + yres;
        case SUB: return xres - yres;
        case MUL: return xres * yres;
        default:; // silence warning
    }
    exit(EXIT_FAILURE); // should never be reached
}

void acirc_eval_mpz_mod (
    mpz_t rop,
    acirc *c,
    acircref root,
    mpz_t *xs,
    mpz_t *ys, // replace the secrets with something
    mpz_t modulus
) {
    acirc_operation op = c->ops[root];
    if (op == XINPUT) {
        mpz_set(rop, xs[c->args[root][0]]);
        return;
    }
    if (op == YINPUT) {
        mpz_set(rop, ys[c->args[root][0]]);
        return;
    }
    mpz_t xres, yres;
    mpz_inits(xres, yres, NULL);
    acirc_eval_mpz_mod(xres, c, c->args[root][0], xs, ys, modulus);
    acirc_eval_mpz_mod(yres, c, c->args[root][1], xs, ys, modulus);
    if (op == ADD) {
        mpz_add(rop, xres, yres);
    } else if (op == SUB) {
        mpz_sub(rop, xres, yres);
    } else if (op == MUL) {
        mpz_mul(rop, xres, yres);
    }
    mpz_mod(rop, rop, modulus);
    mpz_clears(xres, yres, NULL);
}

bool acirc_ensure (acirc *c, bool verbose)
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

static void topo_helper (int ref, acircref *topo, int *seen, int *i, acirc *c)
{
    if (seen[ref]) return;
    acirc_operation op = c->ops[ref];
    if (op == ADD || op == SUB || op == MUL) {
        topo_helper(c->args[ref][0], topo, seen, i, c);
        topo_helper(c->args[ref][1], topo, seen, i, c);
    }
    topo[(*i)++] = ref;
    seen[ref]    = 1;
}

void acirc_topological_order (acircref *topo, acirc *c, acircref ref)
{
    int *seen = acirc_calloc(c->_ref_alloc, sizeof(int));
    int i = 0;
    topo_helper(ref, topo, seen, &i, c);
    free(seen);
}

// dependencies fills an array with the refs to the subcircuit rooted at ref.
// deps is the target array, i is an index into it.
static void dependencies_helper (acircref *deps, bool *seen, int *i, acirc *c, int ref)
{
    if (seen[ref]) return;
    acirc_operation op = c->ops[ref];
    if (op == XINPUT || op == YINPUT) return;
    // otherwise it's an ADD, SUB, or MUL gate
    int xarg = c->args[ref][0];
    int yarg = c->args[ref][1];
    deps[(*i)++] = xarg;
    deps[(*i)++] = yarg;
    dependencies_helper(deps, seen, i, c, xarg);
    dependencies_helper(deps, seen, i, c, yarg);
    seen[ref] = 1;
}

static int dependencies (acircref *deps, acirc *c, int ref)
{
    bool *seen = acirc_calloc(c->nrefs, sizeof(bool));
    int ndeps = 0;
    dependencies_helper(deps, seen, &ndeps, c, ref);
    free(seen);
    return ndeps;
}

acirc_topo_levels* acirc_topological_levels (acirc *c, acircref root)
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

void acirc_topo_levels_destroy (acirc_topo_levels *topo)
{
    for (int i = 0; i < topo->nlevels; i++)
        free(topo->levels[i]);
    free(topo->levels);
    free(topo->level_sizes);
    free(topo);
}

////////////////////////////////////////////////////////////////////////////////
// acirc info calculations

size_t acirc_depth (acirc *c, acircref ref)
{
    acirc_operation op = c->ops[ref];
    if (op == XINPUT || op == YINPUT) {
        return 0;
    }
    size_t xres = acirc_depth(c, c->args[ref][0]);
    size_t yres = acirc_depth(c, c->args[ref][1]);
    return max(xres, yres);
}

size_t acirc_degree (acirc *c, acircref ref)
{
    acirc_operation op = c->ops[ref];
    if (op == XINPUT || op == YINPUT) {
        return 1;
    }
    size_t xres = acirc_degree(c, c->args[ref][0]);
    size_t yres = acirc_degree(c, c->args[ref][1]);
    if (op == MUL)
        return xres + yres;
    return max(xres, yres); // else op == ADD || op == SUB
}

size_t acirc_max_degree (acirc *c)
{
    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; i++) {
        size_t tmp = acirc_degree(c, c->outrefs[i]);
        if (tmp > ret)
            ret = tmp;
    }
    return ret;
}

size_t acirc_var_degree (acirc *c, acircref ref, input_id id)
{
    acirc_operation op = c->ops[ref];
    if (op == XINPUT) {
        return (c->args[ref][0] == id) ? 1 : 0;
    } else if (op == YINPUT) {
        return 0;
    }
    size_t xres = acirc_var_degree(c, c->args[ref][0], id);
    size_t yres = acirc_var_degree(c, c->args[ref][1], id);
    if (op == MUL)
        return xres + yres;
    return max(xres, yres); // else op == ADD || op == SUB
}

size_t acirc_const_degree (acirc *c, acircref ref)
{
    acirc_operation op = c->ops[ref];
    if (op == YINPUT) {
        return 1;
    } else if (op == XINPUT) {
        return 0;
    }
    size_t xres = acirc_const_degree(c, c->args[ref][0]);
    size_t yres = acirc_const_degree(c, c->args[ref][1]);
    if (op == MUL)
        return xres + yres;
    return max(xres, yres); // else op == ADD || op == SUB
}

size_t acirc_max_var_degree (acirc *c, input_id id)
{
    size_t ret = 0;
    for (size_t i = 0; i < c->noutputs; i++) {
        size_t tmp = acirc_var_degree(c, c->outrefs[i], id);
        if (tmp > ret)
            ret = tmp;
    }
    return ret;
}

size_t acirc_max_const_degree (acirc *c)
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
size_t acirc_delta (acirc *c)
{
    size_t delta = acirc_max_const_degree(c);
    for (size_t i = 0; i < c->ninputs; i++) {
        delta += acirc_max_var_degree(c, i);
    }
    return delta;
}

////////////////////////////////////////////////////////////////////////////////
// acirc creation

void acirc_add_test (acirc *c, char *inpstr, char *outstr)
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

void acirc_add_xinput (acirc *c, acircref ref, input_id id)
{
    ensure_gate_space(c, ref);
    c->ninputs += 1;
    c->nrefs   += 1;
    c->ops[ref] = XINPUT;
    acircref *args = acirc_malloc(2 * sizeof(acircref));
    args[0] = id;
    args[1] = -1;
    c->args[ref] = args;
}

void acirc_add_yinput (acirc *c, acircref ref, size_t id, int val)
{
    ensure_gate_space(c, ref);
    if (c->nconsts >= c->_consts_alloc) {
        c->consts = acirc_realloc(c->consts, 2 * c->_consts_alloc * sizeof(int));
        c->_consts_alloc *= 2;
    }
    c->consts[c->nconsts] = val;
    c->nconsts  += 1;
    c->nrefs    += 1;
    c->ops[ref]  = YINPUT;
    acircref *args = acirc_malloc(2 * sizeof(acircref));
    args[0] = id;
    args[1] = val;
    c->args[ref] = args;
}

void acirc_add_gate (acirc *c, acircref ref, acirc_operation op, int xref, int yref, bool is_output)
{
    ensure_gate_space(c, ref);
    c->ngates   += 1;
    c->nrefs    += 1;
    c->ops[ref]  = op;
    acircref *args = acirc_malloc(2 * sizeof(acircref));
    args[0] = xref;
    args[1] = yref;
    c->args[ref] = args;
    if (is_output) {
        if (c->noutputs >= c->_outref_alloc) {
            c->outrefs = acirc_realloc(c->outrefs, 2 * c->_outref_alloc * sizeof(acircref));
            c->_outref_alloc *= 2;
        }
        c->outrefs[c->noutputs++] = ref;
    }
}

////////////////////////////////////////////////////////////////////////////////
// helpers

static void ensure_gate_space (acirc *c, acircref ref)
{
    if (ref >= c->_ref_alloc) {
        c->args = acirc_realloc(c->args, 2 * c->_ref_alloc * sizeof(acircref**));
        c->ops  = acirc_realloc(c->ops,  2 * c->_ref_alloc * sizeof(acirc_operation));
        c->_ref_alloc *= 2;
    }
}

acirc_operation acirc_str2op (char *s)
{
    if (strcmp(s, "ADD") == 0) {
        return ADD;
    } else if (strcmp(s, "SUB") == 0) {
        return SUB;
    } else if (strcmp(s, "MUL") == 0) {
        return MUL;
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

void* acirc_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_calloc] couldn't allocate %lu bytes!\n", nmemb * size);
        assert(false);
    }
    return ptr;
}

void* acirc_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_malloc] couldn't allocate %lu bytes!\n", size);
        assert(false);
    }
    return ptr;
}

void* acirc_realloc(void *ptr, size_t size)
{
    void *ptr_ = realloc(ptr, size);
    if (ptr_ == NULL) {
        fprintf(stderr, "[acirc_realloc] couldn't reallocate %lu bytes!\n", size);
        assert(false);
    }
    return ptr_;
}
