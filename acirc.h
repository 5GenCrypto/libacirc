#ifndef __ACIRC_H__
#define __ACIRC_H__

#include <gmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef unsigned long acircref;
typedef unsigned long input_id;

typedef enum {
    XINPUT,
    YINPUT,
    ADD,
    SUB,
    MUL
} acirc_operation;

typedef struct {
    size_t ninputs;
    size_t nconsts;
    size_t ngates;
    size_t nrefs;
    size_t ntests;
    size_t noutputs;
    acirc_operation *ops;
    acircref **args; // [nextref][2]
    acircref *outrefs;
    int *consts;
    int **testinps;
    int **testouts;
    size_t _ref_alloc; // alloc size of args and ops
    size_t _test_alloc;
    size_t _outref_alloc;
    size_t _consts_alloc;
} acirc;

void acirc_init  (acirc *c);
void acirc_clear (acirc *c);
void acirc_parse (acirc *c, char *filename);
acirc* acirc_from_file (char *filename);

// evaluation
int acirc_eval (acirc *c, acircref ref, int *xs);

void acirc_eval_mpz_mod (
    mpz_t rop,
    acirc *c,
    acircref root,
    mpz_t *xs,
    mpz_t *ys, // replace the secrets with something
    mpz_t modulus
);

// run all tests
bool acirc_ensure (acirc *c, bool verbose);

// topological orderings
void acirc_topological_order (acircref *topo, acirc *c, acircref ref);

typedef struct {
    int nlevels;
    int **levels;
    int *level_sizes;
} acirc_topo_levels;

acirc_topo_levels* acirc_topological_levels (acirc *c, acircref root);
void acirc_topo_levels_destroy (acirc_topo_levels *topo);

// degree calculations
size_t acirc_depth        (acirc *c, acircref ref);
size_t acirc_degree       (acirc *c, acircref ref);
size_t acirc_var_degree   (acirc *c, acircref ref, input_id id);
size_t acirc_const_degree (acirc *c, acircref ref);

size_t acirc_max_degree       (acirc *c);
size_t acirc_max_var_degree   (acirc *c, input_id id);
size_t acirc_max_const_degree (acirc *c);

size_t acirc_delta (acirc *c); // sum of all the var degrees and the const degree, maximized over outputs

// construction
void acirc_add_test   (acirc *c, char *inp, char *out);
void acirc_add_xinput (acirc *c, acircref ref, size_t input_id);
void acirc_add_yinput (acirc *c, acircref ref, size_t const_id, int val);
void acirc_add_gate   (acirc *c, acircref ref, acirc_operation op, int xref, int yref, bool is_output);

// helpers
acirc_operation acirc_str2op (char *s);

#endif
